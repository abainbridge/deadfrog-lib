// Own header
#include "df_window.h"

// Project headers
#include "df_bitmap.h"
#include "df_time.h"

// Platform includes
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>

// Standard includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define FATAL_ERROR(msg, ...) { fprintf(stderr, msg "\n", ##__VA_ARGS__); __asm__("int3"); exit(-1); }

DfWindow *g_window = NULL;
DfInput g_input = { 0 };

static unsigned g_framesThisSecond = 0;
static double g_endOfSecond = 0.0;
static double g_lastUpdateTime = 0.0;


//
// X11 protocol definitions

enum {
    X11_OPCODE_CREATE_WINDOW = 1,
    X11_OPCODE_MAP_WINDOW = 8,
    X11_OPCODE_QUERY_KEYMAP = 44,
    X11_OPCODE_CREATE_GC = 55,
    X11_OPCODE_PUT_IMAGE = 72,

    X11_CW_EVENT_MASK = 1<<11,
    X11_EVENT_MASK_KEY_PRESS = 1,
    X11_EVENT_MASK_POINTER_MOTION = 1<<6,
};


typedef struct __attribute__((packed)) {
    uint8_t order;
    uint8_t pad1;
    uint16_t major_version, minor_version;
    uint16_t auth_proto_name_len;
    uint16_t auth_proto_data_len;
    uint16_t pad2;
} connection_request_t;


typedef struct __attribute__((packed)) {
    uint32_t root_id;
    uint32_t colormap;
    uint32_t white, black;
    uint32_t input_mask;
    uint16_t width, height;
    uint16_t width_mm, height_mm;
    uint16_t maps_min, maps_max;
    uint32_t root_visual_id;
    uint8_t backing_store;
    uint8_t save_unders;
    uint8_t depth;
    uint8_t allowed_depths_len;
} screen_t;


typedef struct __attribute__((packed)) {
    uint8_t depth;
    uint8_t bpp;
    uint8_t scanline_pad;
    uint8_t pad[5];
} pixmap_format_t;


typedef struct __attribute__((packed)) {
    uint32_t release;
    uint32_t id_base, id_mask;
    uint32_t motion_buffer_size;
    uint16_t vendor_len;
    uint16_t request_max;
    uint8_t num_screens;
    uint8_t num_pixmap_formats;
    uint8_t image_byte_order;
    uint8_t bitmap_bit_order;
    uint8_t scanline_unit, scanline_pad;
    uint8_t keycode_min, keycode_max;
    uint32_t pad;
    char vendor_string[1];
} connection_reply_success_body_t;


typedef struct __attribute__((packed)) {
    uint8_t success;
    uint8_t pad;
    uint16_t major_version, minor_version;
    uint16_t len;
} connection_reply_header_t;


typedef struct __attribute__((packed)) {
    uint8_t group;
    uint8_t bits;
    uint16_t colormap_entries;
    uint32_t mask_red, mask_green, mask_blue;
    uint32_t pad;
} visual_t;

// End of X11 protocol definitions
//


typedef struct {
    int socket_fd;
    char recv_buf[10000];
    int recv_buf_num_bytes;

    connection_reply_header_t connection_reply_header;
    connection_reply_success_body_t *connection_reply_success_body;

    pixmap_format_t *pixmap_formats; // Points into connection_reply_success_body.
    screen_t *screens; // Points into connection_reply_success_body.

    uint32_t next_resource_id;
    uint32_t graphics_context_id;
    uint32_t window_id;
} state_t;


static state_t g_state = { .socket_fd = -1 };


static int ConvertX11Keycode(int i) {
    switch (i) {
        case 37:  return KEY_CONTROL;
        case 111: return KEY_UP;
        case 113: return KEY_LEFT;
        case 114: return KEY_RIGHT;
        case 116: return KEY_DOWN;
    }

    return 0;
}


static void HandleErrorEvent() {
    switch (g_state.recv_buf[1]) {
        case 9: printf("Bad drawable\n"); break;
        case 16: printf("Bad length\n"); break;
        default: printf("Unknown error code %i\n", g_state.recv_buf[1]);
    }
    exit(-1);
}


static void ConsumeMessage(int len) {
    memmove(g_state.recv_buf, g_state.recv_buf + len, g_state.recv_buf_num_bytes - len);
    g_state.recv_buf_num_bytes -= len;
    if (g_state.recv_buf_num_bytes > sizeof(g_state.recv_buf)) {
        FATAL_ERROR("bad num bytes");
    }
}


static void send_buf(const void *_buf, int len) {
    const char *buf = (const char *)_buf;
    while (1) {
        struct pollfd poll_fd = { g_state.socket_fd, POLLOUT };
        poll(&poll_fd, 1, -1);

        int size_sent = write(g_state.socket_fd, buf, len);
        if (size_sent < 0) {
            FATAL_ERROR("Couldn't send buf");
        }

        len -= size_sent;
        // TODO, assert that (size_to_send >= 0)
        if (len == 0) break;
        
        buf += size_sent;
    }
}


static void fatal_read(void *buf, size_t count) {
    if (recvfrom(g_state.socket_fd, buf, count, 0, NULL, NULL) != count) {
        FATAL_ERROR("Failed to read.");
    }
}


static void read_response(void *buf, size_t expected_len) {
    while (expected_len) {
        ssize_t len = recv(g_state.socket_fd, g_state.recv_buf, sizeof(g_state.recv_buf), 0);
        if (len == 0) {
            printf("X11 server closed the socket\n");
            continue;
        }
        if (len < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            
            perror("");
            FATAL_ERROR("Couldn't read from socket. Len = %i", (int)len);
        }

        g_state.recv_buf_num_bytes += len;
        if (g_state.recv_buf_num_bytes > sizeof(g_state.recv_buf)) {
            FATAL_ERROR("bad num bytes");
        }

        int keep_going = 1;
        while (keep_going && g_state.recv_buf_num_bytes) {
            int key_code;
            switch (g_state.recv_buf[0]) {
            case 0: HandleErrorEvent(); break;
            case 1:
                // Handle reply.
                if (g_state.recv_buf_num_bytes >= expected_len) {
                    memcpy(buf, g_state.recv_buf, expected_len);
                    ConsumeMessage(expected_len);
                    expected_len = 0;   // Store the fact that we've received the expected reply.
                }
                else {
                    keep_going = 0;
                }
                break;
            case 2:
                if (g_state.recv_buf_num_bytes >= 32) {
                    key_code = ConvertX11Keycode(g_state.recv_buf[1]);
                    g_input.keyDowns[key_code] = 1;
                    ConsumeMessage(8 * 4);
                }
                else {
                    keep_going = 0;
                }
                break;
            case 3:
                if (g_state.recv_buf_num_bytes >= 32) {
                    key_code = ConvertX11Keycode(g_state.recv_buf[1]);
                    g_input.keyUps[key_code] = 1;
                    ConsumeMessage(8 * 4);
                }
                else {
                    keep_going = 0;
                }                    
                break;
            default:
                FATAL_ERROR("Got an unknown message type (%i).\n", g_state.recv_buf[0]);
            }
        }

        if (g_state.recv_buf_num_bytes != 0) {
            FATAL_ERROR("Received incomplete packet");
        }
    }
}


// Initialize the connection to the Xserver if we haven't already.
static void ensure_state() {
    if (g_state.socket_fd >= 0) return;

    // Open socket and connect.
    g_state.socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (g_state.socket_fd < 0) {
        FATAL_ERROR("Create socket failed");
    }
    struct sockaddr_un serv_addr = { 0 };
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, "/tmp/.X11-unix/X0");
    if (connect(g_state.socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        FATAL_ERROR("Couldn't connect");
    }

    // Read Xauthority.
    char xauth_cookie[4096];
    FILE *xauth_file = fopen("/home/andy/.Xauthority", "rb");
    if (!xauth_file) {
        FATAL_ERROR("Couldn't open .Xauthority.");
    }
    size_t xauth_len = fread(xauth_cookie, 1, sizeof(xauth_cookie), xauth_file);
    if (xauth_len < 0) {
        FATAL_ERROR("Couldn't read from .Xauthority.");
    }
    fclose(xauth_file);

    // Send connection request.
    connection_request_t request = { 0 };
    request.order = 'l';  // Little endian.
    request.major_version = 11;
    request.minor_version =  0;
    request.auth_proto_name_len = 18;
    request.auth_proto_data_len = 16;
    send_buf(&request, sizeof(connection_request_t));
    send_buf("MIT-MAGIC-COOKIE-1\0\0", 20);
    send_buf(xauth_cookie + xauth_len - 16, 16);

    // Read connection reply header.
    fatal_read(&g_state.connection_reply_header, sizeof(connection_reply_header_t));
    if (g_state.connection_reply_header.success == 0) {
        FATAL_ERROR("Connection reply indicated failure.");
    }

    // Read rest of connection reply.
    g_state.connection_reply_success_body = (connection_reply_success_body_t*)new char[g_state.connection_reply_header.len * 4];
    fatal_read(g_state.connection_reply_success_body,
               g_state.connection_reply_header.len * 4);

    // Set some pointers into the connection reply because they'll be convenient later.
    g_state.pixmap_formats = (pixmap_format_t *)(g_state.connection_reply_success_body->vendor_string +
                             g_state.connection_reply_success_body->vendor_len);
    g_state.screens = (screen_t *)(g_state.pixmap_formats +
                                  g_state.connection_reply_success_body->num_pixmap_formats);

    g_state.next_resource_id = g_state.connection_reply_success_body->id_base;
}


static uint32_t generate_id() {
    return g_state.next_resource_id++;
}


static void create_gc() {
    g_state.graphics_context_id = generate_id();
    int const len = 4;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_GC | (len<<16);
    packet[1] = g_state.graphics_context_id;
    packet[2] = g_state.window_id;
    packet[3] = 0; // Value mask.

    send_buf(packet, sizeof(packet));
}


static void map_window() {
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_MAP_WINDOW | (len<<16);
    packet[1] = g_state.window_id;
    send_buf(packet, 8);
}


static void InitInput()
{
    g_input.eventSinceAdvance = true;
    g_input.windowHasFocus = true;
}


bool CreateWin(int width, int height, WindowType windowed, char const *winName) {
    DfWindow *wd = g_window = new DfWindow;
	memset(wd, 0, sizeof(DfWindow));
    wd->bmp = BitmapCreate(width, height);

    ensure_state();
    
    g_state.window_id = generate_id();

    int const len = 9;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_WINDOW | (len<<16);
    packet[1] = g_state.window_id;
    packet[2] = g_state.screens[0].root_id;
    packet[3] = 0; // x,y pos. System will position window.
    packet[4] = width | (height<<16);
    packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    packet[6] = 0; // Visual: Copy from parent.
    packet[7] = 0x800; // value_mask = event-mask
    packet[8] = 1 | 2; // event-mask = keypress and key release

    send_buf(packet, sizeof(packet));

    create_gc();
    map_window();

    // Make socket non-blocking.
    int flags = fcntl(g_state.socket_fd, F_GETFL, 0);
    if (flags == -1) {
        FATAL_ERROR("Couldn't get flags of socket");
    }
    flags |= O_NONBLOCK;
    if (fcntl(g_state.socket_fd, F_SETFL, flags) != 0) {
        FATAL_ERROR("Couldn't set socket as non-blocking");
    }

    InitInput();

    return true;
}


void UpdateWin() {
    // *** FPS Meter ***

    g_framesThisSecond++;

    double currentTime = GetRealTime();
    if (currentTime > g_endOfSecond)
    {
        // If program has been paused by a debugger, skip the seconds we missed
        if (currentTime > g_endOfSecond + 2.0)
            g_endOfSecond = currentTime + 1.0;
        else
            g_endOfSecond += 1.0;
        g_window->fps = g_framesThisSecond;
        g_framesThisSecond = 0;
    }
    else if (g_endOfSecond > currentTime + 2.0)
    {
        g_endOfSecond = currentTime + 1.0;
    }


    // *** Advance time ***

    g_window->advanceTime = currentTime - g_lastUpdateTime;
    g_lastUpdateTime = currentTime;


    // *** Send back-buffer to Xserver.
    int W = g_window->bmp->width;
    int H = g_window->bmp->height;
    enum { MAX_BYTES_PER_REQUEST = 262140 }; // Value from www.x.org/releases/X11R7.7/doc/bigreqsproto/bigreq.html
    int num_rows_in_chunk = MAX_BYTES_PER_REQUEST / 4 / W;
    DfColour *row = g_window->bmp->pixels;
    for (int y = 0; y < H; y += num_rows_in_chunk) {
        if (y + num_rows_in_chunk > H) {
            num_rows_in_chunk = H - y;
        }
        
        uint32_t packet[6];   
        uint32_t bmp_format = 2 << 8;
        uint32_t request_len = (uint32_t)(W * num_rows_in_chunk + 6) << 16;
        packet[0] = X11_OPCODE_PUT_IMAGE | bmp_format | request_len;
        packet[1] = g_state.window_id;
        packet[2] = g_state.graphics_context_id;
        packet[3] = W | (num_rows_in_chunk << 16); // Width and height.
        packet[4] = 0 | (y << 16); // Dst X and Y.
        packet[5] = 24 << 8; // Bit depth.

        send_buf(packet, sizeof(packet));
        send_buf(row, W * num_rows_in_chunk * 4);
        row += W * num_rows_in_chunk;
    }
}


bool GetDesktopRes(int *width, int *height) {
    ensure_state();
    *width = g_state.screens[0].width;
    *height = g_state.screens[0].height;
    return true;
}


bool WaitVsync() {
    usleep(16667);
    return true;
}


bool InputPoll()
{
    uint32_t packet = X11_OPCODE_QUERY_KEYMAP | (1<<16);
    send_buf(&packet, sizeof(packet));

    uint8_t resp[40];
    read_response(resp, sizeof(resp));

    for (int i = 0; i < 32; i++) {
        uint8_t bits = resp[8 + i];
//        printf("%02x ", bits);
//         if (bits)
//             puts("Key pressed");
//         
        for (int j = 0; j < 8; j++) {
            int x11_keycode = i * 8 + j;
            int df_keycode = ConvertX11Keycode(x11_keycode);
            g_input.keys[df_keycode] = bits & 1;
            bits >>= 1;
        }
    }

    return true;
}


#if 1
void Sierpinski3DMain();
int main() {
    Sierpinski3DMain();
}
#endif

// Build with:
// gcc main.c -L/usr/X11R6/lib -lX11 -o x11 -g -Wall
// Tested on xubuntu 20.

// Platform includes
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <fcntl.h>
#include <poll.h>

// Standard includes
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define FATAL_ERROR(msg, ...) { fprintf(stderr, msg "\n", ##__VA_ARGS__); exit(-1); }


//
// X11 protocol definitions

enum {
    X11_OPCODE_CREATE_WINDOW = 1,
    X11_OPCODE_MAP_WINDOW = 8,
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

    connection_reply_header_t connection_reply_header;
    connection_reply_success_body_t *connection_reply_success_body;

    pixmap_format_t *pixmap_formats; // Points into connection_reply_success_body.
    screen_t *screens; // Points into connection_reply_success_body.

    uint32_t next_resource_id;
    uint32_t graphics_context_id;
    uint32_t window_id;
} state_t;


static void fatal_write(int fd, const void *buf, size_t count) {
    if (write(fd, buf, count) != count) {
        FATAL_ERROR("Failed to write.");
    }
}


static void fatal_read(int fd, void *buf, size_t count) {
    if (recvfrom(fd, buf, count, 0, NULL, NULL) != count) {
        FATAL_ERROR("Failed to read.");
    }
}


void x11_init(state_t *state) {
    // Open socket and connect.
    state->socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (state->socket_fd < 0) {
        FATAL_ERROR("Create socket failed");
    }
    struct sockaddr_un serv_addr = { 0 };
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, "/tmp/.X11-unix/X0");
    if (connect(state->socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
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
    fatal_write(state->socket_fd, &request, sizeof(connection_request_t));
    fatal_write(state->socket_fd, "MIT-MAGIC-COOKIE-1\0\0", 20);
    fatal_write(state->socket_fd, xauth_cookie + xauth_len - 16, 16);

    // Read connection reply header.
    fatal_read(state->socket_fd, &state->connection_reply_header, sizeof(connection_reply_header_t));
    if (state->connection_reply_header.success == 0) {
        FATAL_ERROR("Connection reply indicated failure.");
    }

    // Read rest of connection reply.
    state->connection_reply_success_body = malloc(state->connection_reply_header.len * 4);
    fatal_read(state->socket_fd, state->connection_reply_success_body,
               state->connection_reply_header.len * 4);

    // Set some pointers into the connection reply because they'll be convenient later.
    state->pixmap_formats = (pixmap_format_t *)(state->connection_reply_success_body->vendor_string +
                             state->connection_reply_success_body->vendor_len);
    state->screens = (screen_t *)(state->pixmap_formats +
                                  state->connection_reply_success_body->num_pixmap_formats);

//     {
//         uint8_t packet[20] = { 0 };
//         packet[0] = 98; // Opcode = QueryExtension.
//         packet[2] = 5;  // Request length in 32-bit words.
//         packet[4] = 12; // Name length.
//         memcpy(packet + 8, "BIG-REQUESTS", 12);
//         fatal_write(state->socket_fd, packet, sizeof(packet));
// 
//         uint8_t reply[32];
//         fatal_read(state->socket_fd, reply, sizeof(reply));
//         if (reply[8] != 1) {
//             FATAL_ERROR("Big requests not supported");
//         }
//     }
// 
//     // Enable big requests
//     if (1)
//     {
//         uint16_t packet[2];
//         packet[0] = 133;
//         packet[1] = 1;
//         fatal_write(state->socket_fd, packet, sizeof(packet));
// 
//         uint8_t buf[64];
//         int len = recvfrom(state->socket_fd, buf, sizeof(buf), 0, NULL, NULL);
//         if (len < 12) {
//             FATAL_ERROR("Big requests response too small");
//         }
//         if (buf[0] != 1) {
//             FATAL_ERROR("Big requests enable failed\n");
//         }
//         state->max_request_size = *((uint32_t *)(buf + 8));
//     }

    state->next_resource_id = state->connection_reply_success_body->id_base;
}


static uint32_t generate_id(state_t *state) {
    return state->next_resource_id++;
}


void create_window(state_t *state, uint16_t w, uint16_t h, uint32_t window_parent) {
    state->window_id = generate_id(state);

    int const len = 9;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_WINDOW | (len<<16);
    packet[1] = state->window_id;
    packet[2] = window_parent;
    packet[3] = 0; // x,y pos. System will position window.
    packet[4] = w | (h<<16);
    packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    packet[6] = 0; // Visual: Copy from parent.
    packet[7] = 0x800; // value_mask = event-mask
    packet[8] = 1; // event-mask = keypress and exposure

    fatal_write(state->socket_fd, packet, sizeof(packet));
}


void create_gc(state_t *state) {
    state->graphics_context_id = generate_id(state);
    int const len = 4;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_GC | (len<<16);
    packet[1] = state->graphics_context_id;
    packet[2] = state->window_id;
    packet[3] = 0; // Value mask.

    fatal_write(state->socket_fd, packet, sizeof(packet));
}


void map_window(state_t *state) {
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_MAP_WINDOW | (len<<16);
    packet[1] = state->window_id;
    fatal_write(state->socket_fd, packet, 8);
}


static void send_block(int fd, char *block, int len) {
    while (1) {
        int size_sent = write(fd, block, len);
        if (size_sent < 0) {
            FATAL_ERROR("Couldn't send block");
        }

        len -= size_sent;
        // TODO, assert that (size_to_send >= 0)
        if (len == 0) break;
        
        block += size_sent;

        struct pollfd poll_fd = { fd, POLLOUT };
        poll(&poll_fd, 1, -1);
    }
}


void put_image(state_t *state) {
    enum { W = 320, H = 240 };
    enum { BITMAP_SIZE_BYTES = W * H * 4 };
    enum { MAX_BYTES_PER_REQUEST = 262144 };

    static uint32_t *bmp = NULL;
    if (!bmp) {
        bmp = malloc(BITMAP_SIZE_BYTES);
    }

    // Draw something on the bitmap
    for (int y = 0; y < H; y++) {
        uint32_t *row = bmp + y * W;
        for (int x = 0; x < W; x++) {
            row[x] = (x << 8) + y;
        }
    }

    int num_rows_in_chunk = MAX_BYTES_PER_REQUEST / 4 / W;
    for (int y = 0; y < H; y += num_rows_in_chunk) {
        if (y + num_rows_in_chunk > H) {
            num_rows_in_chunk = H - y;
        }
        
        uint32_t packet[6];   
        uint32_t bmp_format = 2 << 8;
        uint32_t request_len = (uint32_t)(W * num_rows_in_chunk + 6) << 16;
        packet[0] = X11_OPCODE_PUT_IMAGE | bmp_format | request_len;
        packet[1] = state->window_id;
        packet[2] = state->graphics_context_id;
        packet[3] = W | (num_rows_in_chunk << 16); // Width and height.
        packet[4] = 0 | (y << 16); // Dst X and Y.
        packet[5] = 24 << 8; // Bit depth.

        fatal_write(state->socket_fd, packet, sizeof(packet));
        send_block(state->socket_fd, (char *)bmp, W * num_rows_in_chunk * 4);
    }
}


int main() {
    state_t state = {0};
    x11_init(&state);
    create_window(&state, 320, 240, state.screens[0].root_id);
    create_gc(&state);
    map_window(&state);

    // Make socket non-blocking.
    int flags = fcntl(state.socket_fd, F_GETFL, 0);
    if (flags == -1) {
        FATAL_ERROR("Couldn't get flags of socket");
    }
    flags |= O_NONBLOCK;
    if (fcntl(state.socket_fd, F_SETFL, flags) != 0) {
        FATAL_ERROR("Couldn't set socket as non-blocking");
    }

    while (1) {
        char buf[1024];
        ssize_t len = recvfrom(state.socket_fd, buf, sizeof(buf), 0, NULL, NULL);
        if (len > 0) {
            if (buf[0] == 0) {
                switch (buf[1]) {
                    case 9: printf("Bad drawable\n"); break;
                    case 16: printf("Bad length\n"); break;
                    default: printf("Unknown error code %i\n", buf[1]);
                }
            }
            else {
                printf("Got an event\n");
            }

            break;
        }

        put_image(&state);
        usleep(100 * 1000);
    }
}

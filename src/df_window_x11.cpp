// Platform includes
#include <linux/limits.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>

// Standard includes
#include <stdint.h>
#include <string.h>
#include <unistd.h>


#define FATAL_ERROR(msg, ...) { fprintf(stderr, msg "\n", ##__VA_ARGS__); __asm__("int3"); exit(-1); }

// X11 protocol specs:
//    https://www.x.org/releases/X11R7.7/doc/index.html

// To debug what we send to the xserver, change the following line:
//    strcpy(serv_addr.sun_path, "/tmp/.X11-unix/X0");
// to:
//    strcpy(serv_addr.sun_path, "/tmp/.X11-unix/X1");
//
// Then, in another terminal, run:
//    xtrace -D:1 -d:0 -k -w
//
// To see what another X11 application's traffic looks like:
//    In one terminal, run:
//        xtrace -D:1 -d:0 -k -w
//    In another, run something like:
//        DISPLAY=:1.0 xclock



//
// X11 protocol definitions

enum {
    X11_OPCODE_CREATE_WINDOW = 1,
    X11_OPCODE_MAP_WINDOW = 8,
    X11_OPCODE_CHANGE_PROPERTY = 18,
    X11_OPCODE_QUERY_KEYMAP = 44,
    X11_OPCODE_CREATE_GC = 55,
    X11_OPCODE_PUT_IMAGE = 72,

    X11_CW_EVENT_MASK = 1<<11,
    X11_EVENT_MASK_KEY_PRESS = 1,
    X11_EVENT_MASK_POINTER_MOTION = 1<<6,

    X11_ATOM_WM_NAME = 39
};

enum {
     X11_EVENT_KEYPRESS = 1,
     X11_EVENT_KEYRELEASE = 2,
     X11_EVENT_BUTTONPRESS = 4,
     X11_EVENT_BUTTONRELEASE = 8,
     X11_EVENT_ENTERWINDOW = 0x10,
     X11_EVENT_LEAVEWINDOW = 0x20,
     X11_EVENT_POINTERMOTION = 0x40,
     X11_EVENT_POINTERMOTIONHINT = 0x80,
     X11_EVENT_STRUCTURE_NOTIFY = 0x20000
//     X11_EVENT_RESIZEREDIRECT = 0x40000
//      #x00000100     Button1Motion
//      #x00000200     Button2Motion
//      #x00000400     Button3Motion
//      #x00000800     Button4Motion
//      #x00001000     Button5Motion
//      #x00002000     ButtonMotion
//      #x00004000     KeymapState
//      #x00008000     Exposure
//      #x00010000     VisibilityChange
//      #x00020000     StructureNotify
//      #x00040000     ResizeRedirect
//      #x00080000     SubstructureNotify
//      #x00100000     SubstructureRedirect
//      #x00200000     FocusChange
//      #x00400000     PropertyChange
//      #x00800000     ColormapChange
//      #x01000000     OwnerGrabButton
//      #xFE000000     unused but must be zero
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
    unsigned char recv_buf[10000];
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


static int x11_keycode_to_df_keycode(int i) {
    switch (i) {
        case 9: return KEY_ESC;
        case 10: return KEY_1;
        case 11: return KEY_2;
        case 12: return KEY_3;
        case 13: return KEY_4;
        case 14: return KEY_5;
        case 15: return KEY_6;
        case 16: return KEY_7;
        case 17: return KEY_8;
        case 18: return KEY_9;
        case 19: return KEY_0;
        case 20: return KEY_MINUS;
        case 21: return KEY_EQUALS;
        case 22: return KEY_BACKSPACE;
        case 23: return KEY_TAB;
        case 24: return KEY_Q;
        case 25: return KEY_W;
        case 26: return KEY_E;
        case 27: return KEY_R;
        case 28: return KEY_T;
        case 29: return KEY_Y;
        case 30: return KEY_U;
        case 31: return KEY_I;
        case 32: return KEY_O;
        case 33: return KEY_P;
        case 34: return KEY_OPENBRACE;
        case 35: return KEY_CLOSEBRACE;
        case 36: return KEY_ENTER;
        case 37: return KEY_CONTROL;
        case 38: return KEY_A;
        case 39: return KEY_S;
        case 40: return KEY_D;
        case 41: return KEY_F;
        case 42: return KEY_G;
        case 43: return KEY_H;
        case 44: return KEY_J;
        case 45: return KEY_K;
        case 46: return KEY_L;
        case 47: return KEY_COLON;
        case 48: return KEY_QUOTE;
        case 49: return KEY_BACK_TICK;
        case 50: return KEY_SHIFT;
        case 51: return KEY_TILDE;
        case 52: return KEY_Z;
        case 53: return KEY_X;
        case 54: return KEY_C;
        case 55: return KEY_V;
        case 56: return KEY_B;
        case 57: return KEY_N;
        case 58: return KEY_M;
        case 59: return KEY_COMMA;
        case 60: return KEY_STOP;
        case 61: return KEY_SLASH;
        case 62: return KEY_SHIFT;
        case 63: return KEY_ASTERISK;
        case 64: return KEY_ALT;
        case 65: return KEY_SPACE;
        case 66: return KEY_CAPSLOCK;
        case 67: return KEY_F1;
        case 68: return KEY_F2;
        case 69: return KEY_F3;
        case 70: return KEY_F4;
        case 71: return KEY_F5;
        case 72: return KEY_F6;
        case 73: return KEY_F7;
        case 74: return KEY_F8;
        case 75: return KEY_F9;
        case 76: return KEY_F10;
        case 77: return KEY_NUMLOCK;
        case 79: return KEY_7_PAD;
        case 80: return KEY_8_PAD;
        case 81: return KEY_9_PAD;
        case 82: return KEY_MINUS_PAD;
        case 83: return KEY_4_PAD;
        case 84: return KEY_5_PAD;
        case 85: return KEY_6_PAD;
        case 86: return KEY_PLUS_PAD;
        case 87: return KEY_1_PAD;
        case 88: return KEY_2_PAD;
        case 89: return KEY_3_PAD;
        case 90: return KEY_0_PAD;
        case 91: return KEY_DEL_PAD;
        case 94: return KEY_BACKSLASH;
        case 95: return KEY_F11;
        case 96: return KEY_F12;
        case 104: return KEY_ENTER;
        case 106: return KEY_SLASH_PAD;
        case 110: return KEY_HOME;
        case 111: return KEY_UP;
        case 112: return KEY_PGUP;
        case 113: return KEY_LEFT;
        case 114: return KEY_RIGHT;
        case 115: return KEY_END;
        case 116: return KEY_DOWN;
        case 117: return KEY_PGDN;
        case 118: return KEY_INSERT;
        case 119: return KEY_DEL;
        case 127: return KEY_PAUSE;
    }

    return 0;
}


static void HandleErrorMessage() {
    // See https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html#Encoding::Errors
    printf("Error message from X11 server - ");
    switch (g_state.recv_buf[1]) {
        case 2: printf("Bad value. %x %x", g_state.recv_buf[2], g_state.recv_buf[3]); break;
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
        int poll_result = poll(&poll_fd, 1, -1);
        if (poll_result == -1)
            FATAL_ERROR("Poll gave an error %i", poll_result);

        int size_sent = write(g_state.socket_fd, buf, len);
        if (size_sent < 0) {
            FATAL_ERROR("Couldn't send buf");
        }

        len -= size_sent;

        if (len < 0) FATAL_ERROR("send_buf bug");
        if (len == 0) break;

        buf += size_sent;
    }
}


static void fatal_read(void *buf, size_t count) {
    if (recvfrom(g_state.socket_fd, buf, count, 0, NULL, NULL) != count) {
        FATAL_ERROR("Failed to read.");
    }
}


static char df_keycode_to_ascii(unsigned char keycode, char modifiers) {
    int shift = modifiers & 1;
    int caps_lock = modifiers & 2;
    int ctrl = modifiers & 4;
    int alt = modifiers & 8;
    int num_lock = modifiers & 0x10;
    int windows_key = modifiers & 0x40;

    if (ctrl || alt || windows_key) {
        return 0;
    }

    if (keycode >= KEY_BACKSPACE && keycode <= KEY_ENTER) {
        return keycode;
    }

    if (keycode == KEY_SPACE) {
        return keycode;
    }

    if (keycode == KEY_BACK_TICK) {
        if (shift) {
            return '¬';
        }
        return '`';
    }

    if (keycode >= KEY_0 && keycode <= KEY_9) {
        if (shift) {
            return keycode - 16;   // TODO Fix £.
        }
        return keycode;
    }

    if (keycode >= KEY_A && keycode <= KEY_Z) {
        if (!shift && !caps_lock) {
            return keycode ^ 32;
        }
        return keycode;
    }

    if (keycode >= KEY_0_PAD && keycode <= KEY_9_PAD) {
        if (num_lock) {
            return keycode - 48;
        }
        else {
            return 0;
        }
    }

    if (keycode >= KEY_ASTERISK && keycode <= KEY_SLASH_PAD) {
        if (keycode == KEY_DEL_PAD && !num_lock) {
            return 0;
        }
        return keycode - 64;
    }

    if (shift) {
        switch (keycode) {
            case KEY_COLON: return ':';
            case KEY_EQUALS: return '+';
            case KEY_COMMA: return '<';
            case KEY_MINUS: return '_';
            case KEY_STOP: return '>';
            case KEY_SLASH: return '?';
            case KEY_QUOTE: return '@';
            case KEY_OPENBRACE: return '{';
            case KEY_BACKSLASH: return '|';
            case KEY_CLOSEBRACE: return '}';
            case KEY_TILDE: return '~';
        }
    }
    else {
        switch (keycode) {
            case KEY_COLON: return ';';
            case KEY_EQUALS: return '=';
            case KEY_COMMA: return ',';
            case KEY_MINUS: return '-';
            case KEY_STOP: return '.';
            case KEY_SLASH: return '/';
            case KEY_QUOTE: return '\'';
            case KEY_OPENBRACE: return '[';
            case KEY_BACKSLASH: return '\\';
            case KEY_CLOSEBRACE: return ']';
            case KEY_TILDE: return '#';
        }
    }

    return 0;
}


static void map_window() {
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_MAP_WINDOW | (len<<16);
    packet[1] = g_state.window_id;
    send_buf(packet, 8);
}


// static void unmap_window() {
//     int const len = 2;
//     uint32_t packet[len];
//     packet[0] = X11_OPCODE_UNMAP_WINDOW | (len<<16);
//     packet[1] = g_state.window_id;
//     send_buf(packet, 8);
// }


static bool handle_event() {
    if (g_state.recv_buf_num_bytes < 32) {
        return false;
    }

    switch (g_state.recv_buf[0]) {
    case 2: // KeyPress event.
        {
            unsigned char x11_keycode = g_state.recv_buf[1];
            unsigned char df_keycode = x11_keycode_to_df_keycode(x11_keycode);
            g_priv.m_newKeyDowns[df_keycode] = 1;
            g_input.keys[df_keycode] = 1;
            int modifiers = g_state.recv_buf[28];
            char ascii = df_keycode_to_ascii(df_keycode, modifiers);
    //printf("Key down. x11_keycode:%i. df_keycode:%i. Ascii:%c. Modifiers: 0x%x\n", x11_keycode, df_keycode, ascii, modifiers);

            if (ascii) {
    			g_priv.m_newKeysTyped[g_priv.m_newNumKeysTyped] = ascii;
    			g_priv.m_newNumKeysTyped++;
            }
            break;
        }

    case 3: // KeyRelease event.
        {
            unsigned char x11_keycode = g_state.recv_buf[1];
            unsigned char df_keycode = x11_keycode_to_df_keycode(x11_keycode);
            g_priv.m_newKeyUps[df_keycode] = 1;
            g_input.keys[df_keycode] = 0;
    //printf("Key up. x11_keycode:%i. df_keycode:%i.\n", x11_keycode, df_keycode);
            break;
        }

    case 4: // Mouse down button event (includes scroll motion).
        switch (g_state.recv_buf[1]) {
            case 1:
                g_input.lmbClicked = 1;
    			g_priv.m_lmbPrivate = true;
                break;
            case 2: g_input.mmbClicked = 1; break;
            case 3: g_input.rmbClicked = 1; break;
            case 4: g_input.mouseZ++; break;
            case 5: g_input.mouseZ--; break;
        }
        //printf("down:%i\n", g_state.recv_buf[1]);
        break;

    case 5: // Mouse button up event.
        switch (g_state.recv_buf[1]) {
            case 1:
                g_input.lmbUnClicked = 1;
    			g_priv.m_lmbPrivate = false;
                break;
            case 2: g_input.mmbUnClicked = 1; break;
            case 3: g_input.rmbUnClicked = 1; break;
        }
        //printf("up:%i\n", g_state.recv_buf[1]);
        break;

    case 6: // Pointer motion event.
        {
            int x = g_state.recv_buf[24] + (g_state.recv_buf[25] << 8);
            int y = g_state.recv_buf[26] + (g_state.recv_buf[27] << 8);
            //printf("detail:%i rx=%i ry=%i\n", g_state.recv_buf[1], x, y);
            g_input.mouseX = x;
            g_input.mouseY = y;
            break;
        }

    case 22: // Configure notify event.
        {
            int w = g_state.recv_buf[20] + (g_state.recv_buf[21] << 8);
            int h = g_state.recv_buf[22] + (g_state.recv_buf[23] << 8);
            BitmapDelete(g_window->bmp);
            g_window->bmp = BitmapCreate(w, h);
//             if (g_window->redrawCallback) {
//                 g_window->redrawCallback();
//             }
            break;
        }

    case 150:
        break;

    case 161:
        // Window Manager wants us to close.
        g_window->windowClosed = true;
        break;
        
    default:
        printf("Got an unknown message type (%i).\n", g_state.recv_buf[0]);
    }

    ConsumeMessage(32);
    return true;
}


// This function is the only way that data is received from the X11 server.
// Because the X11 protocol is asynchronous, we might receive events here when
// we are waiting for a response.
//
// buf should be a pointer to an array if you expect a response, otherwise it
// should be NULL.
//
// expected_len is the length of the expected reply. It is ignored if buf == NULL.
//
// Returns true if the expect reply was found and false otherwise.
static bool poll_socket(void *return_buf, size_t expected_len) {
    unsigned char *buf = g_state.recv_buf + g_state.recv_buf_num_bytes;
    ssize_t buf_len = sizeof(g_state.recv_buf) - g_state.recv_buf_num_bytes;
    ssize_t num_bytes_recvd = recv(g_state.socket_fd, buf, buf_len, 0);
    if (num_bytes_recvd == 0) {
        // Treat this as a FATAL_ERROR because if it happened when we were
        // doing a write to the socket, we'd get a SIGPIPE fatal exception. In
        // other words, the Xserver closing the socket will normally cause us
        // to crash anyway.
        FATAL_ERROR("Xserver closed the socket");
    }

    if (num_bytes_recvd < 0) {
        if (errno == EAGAIN) {
            return false;
        }

        perror("");
        printf("Couldn't read from socket. Len = %i.\n", (int)num_bytes_recvd);
        return false;
    }

    g_state.recv_buf_num_bytes += num_bytes_recvd;

    int keep_going = 1;
    while (keep_going && g_state.recv_buf_num_bytes) {
        if (g_state.recv_buf[0] == 0) {
            HandleErrorMessage();
        } else if (g_state.recv_buf[0] == 1) {
            // Handle reply.
            if (g_state.recv_buf_num_bytes >= expected_len) {
                memcpy(return_buf, g_state.recv_buf, expected_len);
                if (expected_len >= 0) {
                    ConsumeMessage(expected_len);
                }
                expected_len = 0;   // Store the fact that we've received the expected reply.
            }
            else {
                keep_going = 0;
            }
        }
        else {
            keep_going = handle_event();
        }
    }

    return true;
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
    char const *home_dir = getenv("HOME");
    size_t home_dir_len = strlen(home_dir);
    char const *xauth_appender = "/.Xauthority";
    size_t xauth_appender_len = sizeof(xauth_appender);
    if (home_dir_len + xauth_appender_len + 1 > PATH_MAX) {
        FATAL_ERROR("HOME too long");
    }
    char xauth_filename[PATH_MAX];
    memcpy(xauth_filename, home_dir, home_dir_len);
    strcpy(xauth_filename + home_dir_len, xauth_appender);
    FILE *xauth_file = fopen(xauth_filename, "rb");
    if (!xauth_file) {
        FATAL_ERROR("Couldn't open '%s'.", xauth_filename);
    }
    char xauth_cookie[4096];
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


bool CreateWin(int width, int height, WindowType winType, char const *winName) {
    return CreateWinPos(0, 0, width, height, winType, winName);
}


static int get_atom_id(char const *atomName) {
    int atomNameLen = strlen(atomName);
    if (atomNameLen > 19) return 0;

    int requestLenWords = (11 + atomNameLen) / 4;
    int requestLenBytes = requestLenWords * 4;
    uint8_t packet[30] = { 0 };
    packet[0] = 16; // opcode = InternAtom.
    packet[1] = 0; // only_if_exists = 0.
    packet[2] = requestLenWords;
    packet[4] = atomNameLen; // Atom name len in bytes.
    memcpy(packet + 8, atomName, atomNameLen);

    send_buf(packet, requestLenBytes);

    uint8_t reply[32];
    while (!poll_socket(reply, sizeof(reply)))
        ;
        
    uint16_t *id = (uint16_t *)(reply + 8);
    return *id;
}


static void enable_delete_window_event() {
    int requestLen = 7;
    uint32_t packet[requestLen];
    packet[0] = X11_OPCODE_CHANGE_PROPERTY | (requestLen << 16);
    packet[1] = g_state.window_id;
    packet[2] = 0x144; // Property = WM_PROTOCOLS.
    packet[3] = 4; // Type = ATOM.
    packet[4] = 32; // Format = 32 bits per item.
    packet[5] = 1; // Length = 1 item.
    packet[6] = 0x143; // Item data = WM_DELETE_WINDOW.

    send_buf(packet, sizeof(packet));
}


bool CreateWinPos(int x, int y, int width, int height, WindowType windowed, char const *winName) {
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
    packet[3] = 0; // x,y pos. System will position window. TODO - use x and y
    packet[4] = width | (height<<16);
    packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    packet[6] = 0; // Visual: Copy from parent.
    packet[7] = 0x800; // value_mask = event-mask
    packet[8] = X11_EVENT_KEYPRESS | X11_EVENT_KEYRELEASE | X11_EVENT_POINTERMOTION |
                X11_EVENT_BUTTONPRESS | X11_EVENT_BUTTONRELEASE | X11_EVENT_STRUCTURE_NOTIFY;

    send_buf(packet, sizeof(packet));

    create_gc();
    map_window();
    SetWindowTitle(winName);
    enable_delete_window_event();

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

//     int clipboard_id = get_atom_id("CLIPBOARD");
//     int string_id = get_atom_id("STRING");
//     int xsel_data_id = get_atom_id("XSEL_DATA");
//     int incr_id = get_atom_id("INCR");
// 
//     usleep(100000);
//     puts("Sending ConvertSelection request");
// 
//     // Send ConvertSelection request.
//     {
//         uint32_t packet[6];
//         packet[0] = 24 | 6 << 16;
//         packet[1] = g_state.window_id; // requestor
//         packet[2] = clipboard_id; // selection
//         packet[3] = string_id; // target
//         packet[4] = xsel_data_id; // property
//         packet[5] = 0; // time
//         send_buf(packet, sizeof(packet));
//     }
// 
//     usleep(100000);
//     puts("poll_socket");
//     poll_socket(NULL, 0);
// 
//     // Send GetProperty request.
//     {
//         puts("sending GetProperty request");
//         uint32_t packet[6];
//         packet[0] = 20 | 6 << 16;
//         packet[1] = g_state.window_id; // window
//         packet[2] = xsel_data_id;   // property
//         packet[3] = 0; // Type = any
//         packet[4] = 0; // offset = 0
//         packet[5] = 0xfffffffful; // length
//         send_buf(packet, sizeof(packet));
//     }
// 
//     usleep(100000);
// 
//     // Get response to GetProperty request.
//     printf("Getting GetProperty response\n");
//     {
//         unsigned char buf[1024];
//         ssize_t num_bytes_recvd = recv(g_state.socket_fd, buf, sizeof(buf), 0);
//             
//         printf("is reply: %i\n", buf[0]);
//         printf("format: %i\n", buf[1]);
// 
//         uint16_t seqNum = *((uint16_t *)(buf + 2));
//         printf("sequence num: %i\n", seqNum);
// 
//         uint32_t replyLen = *((uint32_t *)(buf + 4));
//         printf("reply len: %i\n", replyLen);
// 
//         uint32_t type = *((uint32_t *)(buf + 8));
//         printf("type: %i\n", type);
//         
//         uint32_t bytesAfter = *((uint32_t *)(buf + 12));
//         printf("bytesAfter: %i\n", bytesAfter);
//         
//         uint32_t lenOfValueInFmtUnits = *((uint32_t *)(buf + 16));
//         printf("len of value in fmt units: %i\n", lenOfValueInFmtUnits);
// 
//         // Then 12 bytes unused.
// 
//         uint32_t numBytesLeft = lenOfValueInFmtUnits;
//         while (numBytesLeft > 0) {
//             ssize_t numBytesToRequest = IntMin(sizeof(buf), numBytesLeft);
//             ssize_t numBytesRecvd = recv(g_state.socket_fd, buf, numBytesToRequest, 0);
//             for (int i = 0; i < lenOfValueInFmtUnits; i++) {
//                 if (buf[i] >= 32 && buf[i] < 128) {
//                     putchar(buf[i]);
//                 }
//                 else {
//                     putchar('?');
//                 }
//                 putchar('.');
//             }
//             
//             break;
//         }
//         usleep(100000);
//     }
// 
//     exit(1);
    
    return true;
}


static void BlitBitmapToWindow(DfWindow *wd) {
    // *** Send back-buffer to Xserver.
    int W = wd->bmp->width;
    int H = wd->bmp->height;
    enum { MAX_BYTES_PER_REQUEST = 65535 }; // Len field is 16-bits
    int num_rows_in_chunk = (MAX_BYTES_PER_REQUEST - 6) / W; // -6 because of packet header
    DfColour *row = wd->bmp->pixels;
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
    usleep(6667);
    return true;
}


bool InputPoll() {
    poll_socket(NULL, 0);
    InputPollInternal();
    return true;
}


void SetMouseCursor(MouseCursorType t) {
}


bool IsWindowMaximized() {
    return false;
}


void SetMaximizedState(bool maximize) {
}


void BringWindowToFront() {
}


void SetWindowTitle(char const *title) {
    int headerNumBytes = 24;
    int titleLen = strlen(title);

    uint32_t packet[64] = { 0 };
    int maxTitleLen = sizeof(packet) - headerNumBytes;
    if (titleLen > maxTitleLen)
        titleLen = maxTitleLen;

    int len = (headerNumBytes + titleLen + 3) / 4;
    packet[0] = X11_OPCODE_CHANGE_PROPERTY | (len << 16);
    packet[1] = g_state.window_id;
    packet[2] = X11_ATOM_WM_NAME; // Property
    packet[3] = 0x1f; // Type
    packet[4] = 8; // Format
    packet[5] = titleLen;
    memcpy(&packet[6], title, titleLen);

    send_buf(packet, len * 4);
}


void SetWindowIcon() {}


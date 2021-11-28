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

// All the X11 protocol specs:
//    https://www.x.org/releases/X11R7.7/doc/index.html
// The most important one being:
//    https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html

// To debug what we send to the xserver:
//    In one terminal, run:
//        xtrace -D:1 -d:0 -k -w
//    Then, in another terminal:  
//        Set the DISPLAY env var to ":1"
//        Run CodeTrowel
//
// You will also see other X11 applications' traffic if they are launched with
// the same DISPLAY string.



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
    X11_EVENT_STRUCTURE_NOTIFY = 0x20000,
//     X11_EVENT_RESIZEREDIRECT = 0x40000,
    X11_EVENT_FOCUSCHANGE = 0x200000,
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
    uint8_t num_pixmapFormats;
    uint8_t image_byte_order;
    uint8_t bitmap_bit_order;
    uint8_t scanline_unit, scanline_pad;
    uint8_t keycode_min, keycode_max;
    uint32_t pad;
    char vendor_string[1];
} connectionReplySuccessBody_t;


typedef struct __attribute__((packed)) {
    uint8_t success;
    uint8_t pad;
    uint16_t major_version, minor_version;
    uint16_t len;
} connectionReplyHeader_t;


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
    int socketFd;
    unsigned char recvBuf[10000];
    int recvBufNumBytesAvailable;

    connectionReplyHeader_t connectionReplyHeader;
    connectionReplySuccessBody_t *connectionReplySuccessBody;

    pixmap_format_t *pixmapFormats; // Points into connectionReplySuccessBody.
    screen_t *screens; // Points into connectionReplySuccessBody.

    uint32_t nextResourceId;
    uint32_t graphicsContextId;
    uint32_t windowId;

    int clipboardId;
    int stringId;
    int xselDataId;
} X11State;


static X11State g_state = { .socketFd = -1 };


static int x11KeycodeToDfKeycode(int i) {
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


static char dfKeycodeToAscii(unsigned char keycode, char modifiers) {
    int shift = modifiers & 1;
    int caps_lock = modifiers & 2;
    int ctrl = modifiers & 4;
    int alt = modifiers & 8;
    int numLock = modifiers & 0x10;
    int windowsKey = modifiers & 0x40;

    if (ctrl || alt || windowsKey) {
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
            switch (keycode) {
               case KEY_0: return ')';
               case KEY_1: return '!';
               case KEY_2: return '"';
               case KEY_3: return '£';
               case KEY_4: return '$';
               case KEY_5: return '%';
               case KEY_6: return '^';
               case KEY_7: return '&';
               case KEY_8: return '*';
               case KEY_9: return '(';
            }
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
        if (numLock) {
            return keycode - 48;
        }
        else {
            return 0;
        }
    }

    if (keycode >= KEY_ASTERISK && keycode <= KEY_SLASH_PAD) {
        if (keycode == KEY_DEL_PAD && !numLock) {
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


// ****************************************************************************
// Socket handling code.
// ****************************************************************************


// This function is the only way that data is received from the X11 server. It:
//
// 1. Uses a large(ish) buffer on the heap to receive into. Callers read from
// that buffer directly, rather than having to allocate their own.
//
// 2. In combination with ConsumeMessage, it allows PDUs that are split across
// a recv() to be remerged into a single contiguous block.
//
// Returns the number of bytes received.
static int ReadFromXServer() {
    unsigned char *buf = g_state.recvBuf + g_state.recvBufNumBytesAvailable;
    ssize_t bufLen = sizeof(g_state.recvBuf) - g_state.recvBufNumBytesAvailable;
    ssize_t numBytesRecvd = recv(g_state.socketFd, buf, bufLen, 0);
    if (numBytesRecvd == 0) {
        // Treat this as a FATAL_ERROR because if it happened when we were
        // doing a write to the socket, we'd get a SIGPIPE fatal exception. In
        // other words, the Xserver closing the socket will normally cause us
        // to crash anyway.
        FATAL_ERROR("Xserver closed the socket");
    }

    if (numBytesRecvd < 0) {
        if (errno == EAGAIN) {
            return 0;
        }

        perror("");
        printf("Couldn't read from socket. Len = %i.\n", (int)numBytesRecvd);
        return 0;
    }

    g_state.recvBufNumBytesAvailable += numBytesRecvd;

    return numBytesRecvd;
}


static void ConsumeMessage(int len) {
    memmove(g_state.recvBuf, g_state.recvBuf + len, g_state.recvBufNumBytesAvailable - len);
    g_state.recvBufNumBytesAvailable -= len;
    if (g_state.recvBufNumBytesAvailable > sizeof(g_state.recvBuf)) {
        FATAL_ERROR("bad num bytes");
    }
}


static void SendBuf(const void *_buf, int len) {
    const char *buf = (const char *)_buf;
    while (1) {
        struct pollfd pollFd = { g_state.socketFd, POLLOUT };
        int pollResult = poll(&pollFd, 1, -1);
        if (pollResult == -1)
            FATAL_ERROR("Poll gave an error %i", pollResult);

        int sizeSent = write(g_state.socketFd, buf, len);
        if (sizeSent < 0) {
            FATAL_ERROR("Couldn't send buf");
        }

        len -= sizeSent;

        if (len < 0) FATAL_ERROR("SendBuf bug");
        if (len == 0) break;

        buf += sizeSent;
    }
}


static void FatalRead(void *buf, size_t count) {
    if (recvfrom(g_state.socketFd, buf, count, 0, NULL, NULL) != count) {
        FATAL_ERROR("Failed to read.");
    }
}


static void HandleErrorMessage() {
    // See https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html#Encoding::Errors
    printf("Error message from X11 server - ");
    switch (g_state.recvBuf[1]) {
        case 2: printf("Bad value. %x %x", g_state.recvBuf[2], g_state.recvBuf[3]); break;
        case 9: printf("Bad drawable\n"); break;
        case 16: printf("Bad length\n"); break;
        default: printf("Unknown error code %i\n", g_state.recvBuf[1]);
    }
    exit(-1);
}


static bool IsEventPending() {
    if (g_state.recvBufNumBytesAvailable >= 2 && g_state.recvBuf[0] == 0)
        return true; // Error message event is pending.

    if (g_state.recvBuf[0] == 1)
        return false;   // Reply is pending.

    if (g_state.recvBufNumBytesAvailable >= 32)
        return true;

    return false;
}


static void handle_event() {
    if (g_state.recvBuf[0] == 1) {
        FATAL_ERROR("Got unexpected reply.");
    }

    switch (g_state.recvBuf[0]) {
    case 2: // KeyPress event.
        {
            unsigned char x11_keycode = g_state.recvBuf[1];
            unsigned char df_keycode = x11KeycodeToDfKeycode(x11_keycode);
            g_priv.m_newKeyDowns[df_keycode] = 1;
            g_input.keys[df_keycode] = 1;
            int modifiers = g_state.recvBuf[28];
            char ascii = dfKeycodeToAscii(df_keycode, modifiers);
    //printf("Key down. x11_keycode:%i. df_keycode:%i. Ascii:%c. Modifiers: 0x%x\n", x11_keycode, df_keycode, ascii, modifiers);

            if (ascii) {
    			g_priv.m_newKeysTyped[g_priv.m_newNumKeysTyped] = ascii;
    			g_priv.m_newNumKeysTyped++;
            }
            break;
        }

    case 3: // KeyRelease event.
        {
            unsigned char x11_keycode = g_state.recvBuf[1];
            unsigned char df_keycode = x11KeycodeToDfKeycode(x11_keycode);
            g_priv.m_newKeyUps[df_keycode] = 1;
            g_input.keys[df_keycode] = 0;
    //printf("Key up. x11_keycode:%i. df_keycode:%i.\n", x11_keycode, df_keycode);
            break;
        }

    case 4: // Mouse down button event (includes scroll motion).
        switch (g_state.recvBuf[1]) {
            case 1:
                g_input.lmb = 1;
    			g_priv.m_lmbPrivate = true;
                break;
            case 2: g_input.mmb = 1; break;
            case 3: g_input.rmb = 1; break;
            case 4: g_input.mouseZ++; break;
            case 5: g_input.mouseZ--; break;
        }
        //printf("down:%i\n", g_state.recvBuf[1]);
        break;

    case 5: // Mouse button up event.
        switch (g_state.recvBuf[1]) {
            case 1:
                g_input.lmb = 0;
    			g_priv.m_lmbPrivate = false;
                break;
            case 2: g_input.mmb = 0; break;
            case 3: g_input.rmb = 0; break;
        }
        //printf("up:%i\n", g_state.recvBuf[1]);
        break;

    case 6: // Pointer motion event.
        {
            int x = g_state.recvBuf[24] + (g_state.recvBuf[25] << 8);
            int y = g_state.recvBuf[26] + (g_state.recvBuf[27] << 8);
            //printf("detail:%i rx=%i ry=%i\n", g_state.recvBuf[1], x, y);
            g_input.mouseX = x;
            g_input.mouseY = y;
            break;
        }

    case 9: // Focus in event.
        HandleFocusInEvent();
        break;
        
    case 10: // Focus out event.
        HandleFocusOutEvent();
        break;

    case 22: // Configure notify event.
        {
            int w = g_state.recvBuf[20] + (g_state.recvBuf[21] << 8);
            int h = g_state.recvBuf[22] + (g_state.recvBuf[23] << 8);
            BitmapDelete(g_window->bmp);
            g_window->bmp = BitmapCreate(w, h);
//             if (g_window->redrawCallback) {
//                 g_window->redrawCallback();
//             }
            break;
        }

    case 31: // Selection Notify
        // This event is only (I hope) received in response to a ConvertSelection
        // request we sent to the server as the start of the just-gimme-the-damn-clipboard-data
        // dance.
        printf("Got selection notify\n");
        ConsumeMessage(32);
        ReceiveClipboardData();
        return;

    case 33: // Client message.
        // We only ever get this when the Window Manager wants us to close.
        g_window->windowClosed = true;
        break;
        
    default:
        printf("Got an unknown message type (%i).\n", g_state.recvBuf[0]);
    }

    ConsumeMessage(32);
}


static bool HandleEvents() {
    ReadFromXServer();

    bool rv = false;
    while (IsEventPending()) {
        handle_event();
        rv = true;
    }

    return rv;
}


// Because the X11 protocol is asynchronous, we might receive events here when
// we are waiting for a response.
//
// Returns true if an event was found and false otherwise.
static bool GetReply(int expectedLen) {
    HandleEvents();

    if (g_state.recvBuf[0] == 1 && g_state.recvBufNumBytesAvailable >= expectedLen)
        return true;

    return false;
}


// ****************************************************************************
// End of socket handling code.
// ****************************************************************************


static void MapWindow() {
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_MAP_WINDOW | (len<<16);
    packet[1] = g_state.windowId;
    SendBuf(packet, 8);
}


static int GetAtomId(char const *atomName) {
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

    SendBuf(packet, requestLenBytes);

    while (!GetReply(32))
        ;
        
    uint16_t *id = (uint16_t *)(g_state.recvBuf + 8);

    ConsumeMessage(32);
    
    return *id;
}


// Initialize the connection to the Xserver if we haven't already.
static void EnsureState() {
    if (g_state.socketFd >= 0) return;

    char *displayString = getenv("DISPLAY");
    char socketName[] = "/tmp/.X11-unix/X0";
    if (displayString && displayString[0] == ':' && isdigit(displayString[1]))
        socketName[16] = displayString[1];

    // Open socket and connect.
    g_state.socketFd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (g_state.socketFd < 0) {
        FATAL_ERROR("Create socket failed");
    }
    struct sockaddr_un servAddr = { 0 };
    servAddr.sun_family = AF_UNIX;
    strcpy(servAddr.sun_path, socketName);
    if (connect(g_state.socketFd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        FATAL_ERROR("Couldn't connect");
    }

    // Read Xauthority.
    char const *homeDir = getenv("HOME");
    size_t homeDirLen = strlen(homeDir);
    char const *xauth_appender = "/.Xauthority";
    size_t xauth_appender_len = sizeof(xauth_appender);
    if (homeDirLen + xauth_appender_len + 1 > PATH_MAX) {
        FATAL_ERROR("HOME too long");
    }
    char xauthFilename[PATH_MAX];
    memcpy(xauthFilename, homeDir, homeDirLen);
    strcpy(xauthFilename + homeDirLen, xauth_appender);
    FILE *xauthFile = fopen(xauthFilename, "rb");
    if (!xauthFile) {
        FATAL_ERROR("Couldn't open '%s'.", xauthFilename);
    }
    char xauthCookie[4096];
    size_t xauthLen = fread(xauthCookie, 1, sizeof(xauthCookie), xauthFile);
    if (xauthLen < 0) {
        FATAL_ERROR("Couldn't read from .Xauthority.");
    }
    fclose(xauthFile);

    // Send connection request.
    connection_request_t request = { 0 };
    request.order = 'l';  // Little endian.
    request.major_version = 11;
    request.minor_version =  0;
    request.auth_proto_name_len = 18;
    request.auth_proto_data_len = 16;
    SendBuf(&request, sizeof(connection_request_t));
    SendBuf("MIT-MAGIC-COOKIE-1\0\0", 20);
    SendBuf(xauthCookie + xauthLen - 16, 16);

    // Read connection reply header.
    FatalRead(&g_state.connectionReplyHeader, sizeof(connectionReplyHeader_t));
    if (g_state.connectionReplyHeader.success == 0) {
        FATAL_ERROR("Connection reply indicated failure.");
    }

    // Read rest of connection reply.
    g_state.connectionReplySuccessBody = (connectionReplySuccessBody_t*)new char[g_state.connectionReplyHeader.len * 4];
    FatalRead(g_state.connectionReplySuccessBody,
               g_state.connectionReplyHeader.len * 4);

    // Set some pointers into the connection reply because they'll be convenient later.
    g_state.pixmapFormats = (pixmap_format_t *)(g_state.connectionReplySuccessBody->vendor_string +
                             g_state.connectionReplySuccessBody->vendor_len);
    g_state.screens = (screen_t *)(g_state.pixmapFormats +
                                  g_state.connectionReplySuccessBody->num_pixmapFormats);

    g_state.nextResourceId = g_state.connectionReplySuccessBody->id_base;

    // Get some Atom ids that we will use later.
    g_state.clipboardId = GetAtomId("CLIPBOARD");
    g_state.stringId = GetAtomId("STRING");
    g_state.xselDataId = GetAtomId("XSEL_DATA");
    printf("Atoms: %d %d %d\n", g_state.clipboardId, g_state.stringId, g_state.xselDataId);
}


static uint32_t generateId() {
    return g_state.nextResourceId++;
}


static void CreateGc() {
    g_state.graphicsContextId = generateId();
    int const len = 4;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_GC | (len<<16);
    packet[1] = g_state.graphicsContextId;
    packet[2] = g_state.windowId;
    packet[3] = 0; // Value mask.

    SendBuf(packet, sizeof(packet));
}


bool CreateWin(int width, int height, WindowType winType, char const *winName) {
    return CreateWinPos(0, 0, width, height, winType, winName);
}


static void EnableDeleteWindowEvent() {
    // See InterClient Communication Conventions Manual v2.0. The section
    // heading is "Window Deletion".
    int requestLen = 7;
    uint32_t packet[requestLen];
    packet[0] = X11_OPCODE_CHANGE_PROPERTY | (requestLen << 16);
    packet[1] = g_state.windowId;
    packet[2] = 0x144; // Property = WM_PROTOCOLS.
    packet[3] = 4; // Type = ATOM.
    packet[4] = 32; // Format = 32 bits per item.
    packet[5] = 1; // Length = 1 item.
    packet[6] = 0x143; // Item data = WM_DELETE_WINDOW.

    SendBuf(packet, sizeof(packet));
}


bool CreateWinPos(int x, int y, int width, int height, WindowType windowed, char const *winName) {
    DfWindow *wd = g_window = new DfWindow;
	memset(wd, 0, sizeof(DfWindow));
    wd->bmp = BitmapCreate(width, height);

    EnsureState();

    g_state.windowId = generateId();

    int const len = 9;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_WINDOW | (len<<16);
    packet[1] = g_state.windowId;
    packet[2] = g_state.screens[0].root_id;
    packet[3] = 0; // x,y pos. System will position window. TODO - use x and y
    packet[4] = width | (height<<16);
    packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    packet[6] = 0; // Visual: Copy from parent.
    packet[7] = 0x800; // value_mask = event-mask
    packet[8] = X11_EVENT_KEYPRESS | X11_EVENT_KEYRELEASE | X11_EVENT_POINTERMOTION |
                X11_EVENT_BUTTONPRESS | X11_EVENT_BUTTONRELEASE | X11_EVENT_STRUCTURE_NOTIFY |
                X11_EVENT_FOCUSCHANGE;

    SendBuf(packet, sizeof(packet));

    CreateGc();
    MapWindow();
    SetWindowTitle(winName);
    EnableDeleteWindowEvent();

    // Make socket non-blocking.
    int flags = fcntl(g_state.socketFd, F_GETFL, 0);
    if (flags == -1) {
        FATAL_ERROR("Couldn't get flags of socket");
    }
    flags |= O_NONBLOCK;
    if (fcntl(g_state.socketFd, F_SETFL, flags) != 0) {
        FATAL_ERROR("Couldn't set socket as non-blocking");
    }

    InitInput();

//     int clipboardId = GetAtomId("CLIPBOARD");
//     int stringId = GetAtomId("STRING");
//     int xselDataId = GetAtomId("XSEL_DATA");
//     printf("%d %d %d\n", clipboardId, stringId, xselDataId);
// 
//     usleep(100000);
//     puts("Sending ConvertSelection request");
// 
//     // Send ConvertSelection request.
//     {
//         uint32_t packet[6];
//         packet[0] = 24 | 6 << 16;
//         packet[1] = g_state.windowId; // requestor
//         packet[2] = clipboardId; // selection
//         packet[3] = stringId; // target
//         packet[4] = xselDataId; // property
//         packet[5] = 0; // time
//         SendBuf(packet, sizeof(packet));
//     }
// 
//     // Wait for long enough that the SelectionNotify response will have arrived.
//     usleep(100000);
// 
//     // Consume it, and assume it tells us that there is a selection.
//     HandleEvents();
// 
//     // Send GetProperty request.
//     {
//         puts("sending GetProperty request");
//         uint32_t packet[6];
//         packet[0] = 20 | 6 << 16;
//         packet[1] = g_state.windowId; // window
//         packet[2] = xselDataId;   // property
//         packet[3] = 0; // Type = any
//         packet[4] = 0; // offset = 0
//         packet[5] = 0xfffffffful; // length
//         SendBuf(packet, sizeof(packet));
//     }
// 
//     // Get response to GetProperty request.
//     printf("Getting GetProperty response\n");
//     {
//         while (!GetReply(32))
//             ;
// 
//         uint8_t *buf = g_state.recvBuf;
//         uint32_t type = *((uint32_t *)(buf + 8));
//         if (type == g_state.stringId) {
//             uint32_t replyLen = *((uint32_t *)(buf + 4)) * 4;
//         
//             uint32_t lenOfValueInFmtUnits = *((uint32_t *)(buf + 16));
//             printf("len of value in fmt units: %i\n", lenOfValueInFmtUnits);
// 
//             ConsumeMessage(32);
// 
//             uint32_t numBytesLeft = lenOfValueInFmtUnits;
//             while (numBytesLeft > 0) {
//                 ReadFromXServer();
// 
//                 ssize_t stringLen = IntMin(g_state.recvBufNumBytesAvailable, numBytesLeft);
//                 for (int i = 0; i < stringLen; i++) {
//                     if (buf[i] >= 32 && buf[i] < 128) {
//                         putchar(buf[i]);
//                     }
//                     else {
//                         putchar('?');
//                     }
//                 }
// 
//                 ConsumeMessage(stringLen);
//                 numBytesLeft -= stringLen;
//             }
// 
//             uint32_t amtPadding = replyLen - lenOfValueInFmtUnits;
//             ConsumeMessage(amtPadding);
//         }
//     }
// 
//    exit(1);
    
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
        packet[1] = g_state.windowId;
        packet[2] = g_state.graphicsContextId;
        packet[3] = W | (num_rows_in_chunk << 16); // Width and height.
        packet[4] = 0 | (y << 16); // Dst X and Y.
        packet[5] = 24 << 8; // Bit depth.

        SendBuf(packet, sizeof(packet));
        SendBuf(row, W * num_rows_in_chunk * 4);
        row += W * num_rows_in_chunk;
    }
}


bool GetDesktopRes(int *width, int *height) {
    EnsureState();
    *width = g_state.screens[0].width;
    *height = g_state.screens[0].height;
    return true;
}


bool WaitVsync() {
    usleep(6667);
    return true;
}


bool InputPoll() {
    bool rv = HandleEvents();
    InputPollInternal();
    return rv;
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
    packet[1] = g_state.windowId;
    packet[2] = X11_ATOM_WM_NAME; // Property
    packet[3] = 0x1f; // Type
    packet[4] = 8; // Format
    packet[5] = titleLen;
    memcpy(&packet[6], title, titleLen);

    SendBuf(packet, len * 4);
}


void SetWindowIcon() {}


void ClipboardRequestData() {
}

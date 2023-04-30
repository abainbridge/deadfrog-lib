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
    X11_OPCODE_SET_SELECTION_OWNER = 22,
    X11_OPCODE_QUERY_KEYMAP = 44,
    X11_OPCODE_CREATE_GC = 55,
    X11_OPCODE_PUT_IMAGE = 72,

    X11_EVENT_CODE_SELECTION_NOTIFY = 31,

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


struct WindowPlatformSpecific {
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

    // Atom values.
    int clipboardId;
    int stringId;
    int xselDataId;
    int targetsId;
    int wmDeleteWindowId;
    int wmProtocolsId;

    char *clipboardRxData;  // NULL except between calls of X11InternalClipboardRequestData() and ClipboardReleaseReceivedData()
    char *clipboardTxData;
    unsigned clipboardTxDataNumChars;
};


// Not a visible window. Just used to store a connection to the XServer. Used
// for things like GetDesktopRes() and the clipboard routines which need to
// even if not real window exists.
static DfWindow g_fakeWindow = { 0 };


static void ReceiveClipboardData(DfWindow *);
static void SendChangePropertyRequest(WindowPlatformSpecific *, uint32_t destWindow, uint32_t target, uint32_t property);
static void SendSendEventSelectionNotify(WindowPlatformSpecific *, uint32_t destWindow, uint32_t target, uint32_t property, uint32_t time);


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
        case 105: return KEY_CONTROL;
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
            case KEY_DEL: return 127;
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
static int ReadFromXServer(WindowPlatformSpecific *platSpec) {
    unsigned char *buf = platSpec->recvBuf + platSpec->recvBufNumBytesAvailable;
    ssize_t bufLen = sizeof(platSpec->recvBuf) - platSpec->recvBufNumBytesAvailable;
    ssize_t numBytesRecvd = recv(platSpec->socketFd, buf, bufLen, 0);
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

    platSpec->recvBufNumBytesAvailable += numBytesRecvd;

    return numBytesRecvd;
}


static void ConsumeMessage(WindowPlatformSpecific *platSpec, int len) {
    memmove(platSpec->recvBuf, platSpec->recvBuf + len, platSpec->recvBufNumBytesAvailable - len);
    platSpec->recvBufNumBytesAvailable -= len;
    if (platSpec->recvBufNumBytesAvailable > sizeof(platSpec->recvBuf)) {
        FATAL_ERROR("bad num bytes");
    }
}


static void SendBuf(WindowPlatformSpecific *platSpec, const void *_buf, int len) {
    const char *buf = (const char *)_buf;
    while (1) {
        struct pollfd pollFd = { platSpec->socketFd, POLLOUT };
        int pollResult = poll(&pollFd, 1, -1);
        if (pollResult == -1)
            FATAL_ERROR("Poll gave an error %i", pollResult);

        int sizeSent = write(platSpec->socketFd, buf, len);
        if (sizeSent < 0) {
            FATAL_ERROR("Couldn't send buf");
        }

        len -= sizeSent;

        if (len < 0) FATAL_ERROR("SendBuf bug");
        if (len == 0) break;

        buf += sizeSent;
    }
}


static void FatalRead(WindowPlatformSpecific *platSpec, void *buf, size_t count) {
    if (recvfrom(platSpec->socketFd, buf, count, 0, NULL, NULL) != count) {
        FATAL_ERROR("Failed to read.");
    }
}


// static void HandleErrorMessage(WindowPlatformSpecific *platSpec) {
//     // See https://www.x.org/releases/X11R7.7/doc/xproto/x11protocol.html#Encoding::Errors
//     printf("Error message from X11 server - ");
//     switch (platSpec->recvBuf[1]) {
//         case 2: printf("Bad value. %x %x", platSpec->recvBuf[2], platSpec->recvBuf[3]); break;
//         case 9: printf("Bad drawable\n"); break;
//         case 16: printf("Bad length\n"); break;
//         default: printf("Unknown error code %i\n", platSpec->recvBuf[1]);
//     }
//     exit(-1);
// }


static bool IsEventPending(WindowPlatformSpecific *platSpec) {
    if (platSpec->recvBufNumBytesAvailable >= 2 && platSpec->recvBuf[0] == 0)
        return true; // Error message event is pending.

    if (platSpec->recvBuf[0] == 1)
        return false;   // Reply is pending.

    if (platSpec->recvBufNumBytesAvailable >= 32)
        return true;

    return false;
}


static uint32_t GetU32FromRecvBuf(WindowPlatformSpecific *platSpec, int offset) {
    return platSpec->recvBuf[offset] +
        (platSpec->recvBuf[offset + 1] << 8) +
        (platSpec->recvBuf[offset + 2] << 16) +
        (platSpec->recvBuf[offset + 3] << 24);
}


static void HandleEvent(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (platSpec->recvBuf[0] == 1) {
        FATAL_ERROR("Got unexpected reply.");
    }

    bool selectionNotifyReceived = false;

    platSpec->recvBuf[0] &= 0x7f; // Clear the seemingly useless "Generated" flag.

    switch (platSpec->recvBuf[0]) {
    case 2: // KeyPress event.
        {
            unsigned char x11_keycode = platSpec->recvBuf[1];
            unsigned char df_keycode = x11KeycodeToDfKeycode(x11_keycode);
            win->_private->newKeyDowns[df_keycode] = 1;
            win->input.keys[df_keycode] = 1;
            int modifiers = platSpec->recvBuf[28];
            char ascii = dfKeycodeToAscii(df_keycode, modifiers);
    //printf("Key down. x11_keycode:%i. df_keycode:%i. Ascii:%c. Modifiers: 0x%x\n", x11_keycode, df_keycode, ascii, modifiers);

            if (ascii) {
                win->_private->newKeysTyped[win->_private->newNumKeysTyped] = ascii;
                win->_private->newNumKeysTyped++;
            }
            break;
        }

    case 3: // KeyRelease event.
        {
            unsigned char x11_keycode = platSpec->recvBuf[1];
            unsigned char df_keycode = x11KeycodeToDfKeycode(x11_keycode);
            win->_private->newKeyUps[df_keycode] = 1;
            win->input.keys[df_keycode] = 0;
    //printf("Key up. x11_keycode:%i. df_keycode:%i.\n", x11_keycode, df_keycode);
            break;
        }

    case 4: // Mouse down button event (includes scroll motion).
        switch (platSpec->recvBuf[1]) {
            case 1:
                win->input.lmb = 1;
                win->_private->lmbPrivate = true;
                break;
            case 2: win->input.mmb = 1; break;
            case 3: win->input.rmb = 1; break;
            case 4: win->input.mouseZ++; break;
            case 5: win->input.mouseZ--; break;
        }
        //printf("down:%i\n", platSpec->recvBuf[1]);
        break;

    case 5: // Mouse button up event.
        switch (platSpec->recvBuf[1]) {
            case 1:
                win->input.lmb = 0;
                win->_private->lmbPrivate = false;
                break;
            case 2: win->input.mmb = 0; break;
            case 3: win->input.rmb = 0; break;
        }
        //printf("up:%i\n", platSpec->recvBuf[1]);
        break;

    case 6: // Pointer motion event.
        {
            int16_t *x = (int16_t*)&platSpec->recvBuf[24];
            int16_t *y = (int16_t*)&platSpec->recvBuf[26];
            //printf("detail:%i rx=%i ry=%i\n", platSpec->recvBuf[1], *x, *y);
            win->input.mouseX = *x;
            win->input.mouseY = *y;
            break;
        }

    case 9: // Focus in event.
        HandleFocusInEvent(win);
        break;

    case 10: // Focus out event.
        HandleFocusOutEvent(win);
        break;

    case 22: // Configure notify event.
        {
            int w = platSpec->recvBuf[20] + (platSpec->recvBuf[21] << 8);
            int h = platSpec->recvBuf[22] + (platSpec->recvBuf[23] << 8);
            BitmapDelete(win->bmp);
            win->bmp = BitmapCreate(w, h);
//             if (win->redrawCallback) {
//                 win->redrawCallback();
//             }
            break;
        }

    case 30: // Selection Request
        {
            uint32_t time = GetU32FromRecvBuf(platSpec, 4);
            uint32_t requestor = GetU32FromRecvBuf(platSpec, 12);
            uint32_t selection = GetU32FromRecvBuf(platSpec, 16);
            ReleaseAssert(selection == platSpec->clipboardId, "selection was %x", selection);
            uint32_t target = GetU32FromRecvBuf(platSpec, 20);
            uint32_t property = GetU32FromRecvBuf(platSpec, 24);
            printf("Recv'd Selection request. Time=%x Requestor=%x target=%x property=%x\n",
                time, requestor, target, property);

            SendChangePropertyRequest(platSpec, requestor, target, property);
            SendSendEventSelectionNotify(platSpec, requestor, target, property, time);
        }
        break;

    case X11_EVENT_CODE_SELECTION_NOTIFY: // Selection Notify
        // This event is only (I hope) received in response to a ConvertSelection
        // request we sent to the server as the start of the just-gimme-the-damn-clipboard-data
        // dance.
//        printf("Got selection notify\n");
        selectionNotifyReceived = true;
        break;

    case 33: // Client message.
        // We only ever get this when the Window Manager wants us to close.
        win->windowClosed = true;
        break;

    default:
        printf("Got an unknown message type (%d).\n", platSpec->recvBuf[0]);
    }

    ConsumeMessage(platSpec, 32);

    if (selectionNotifyReceived) {
        ReceiveClipboardData(win);
    }
}


static bool HandleEvents(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    ReadFromXServer(platSpec);

    bool rv = false;
    while (IsEventPending(platSpec)) {
        HandleEvent(win);
        rv = true;
    }

    return rv;
}


// Because the X11 protocol is asynchronous, we might receive events here when
// we are waiting for a response.
//
// Returns true if an event was found and false otherwise.
static bool GetReply(DfWindow *win, int expectedLen) {
    HandleEvents(win);

    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (platSpec->recvBuf[0] == 1 && platSpec->recvBufNumBytesAvailable >= expectedLen)
        return true;

    return false;
}


// ****************************************************************************
// End of socket handling code.
// ****************************************************************************


static void MapWindow(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_MAP_WINDOW | (len<<16);
    packet[1] = platSpec->windowId;
    SendBuf(platSpec, packet, 8);
}


static int GetAtomId(DfWindow *win, char const *atomName) {
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

    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    SendBuf(platSpec, packet, requestLenBytes);

    while (!GetReply(win, 32))
        ;

    uint16_t *id = (uint16_t *)(platSpec->recvBuf + 8);

    ConsumeMessage(platSpec, 32);

    return *id;
}


static uint32_t generateId(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    return platSpec->nextResourceId++;
}


// Initialize the connection to the Xserver if we haven't already.
static void ConnectToXserver(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (platSpec->socketFd >= 0) return;

    char *displayString = getenv("DISPLAY");
    char socketName[] = "/tmp/.X11-unix/X0";
    if (displayString && displayString[0] == ':' && isdigit(displayString[1]))
        socketName[16] = displayString[1];

    // Open socket and connect.
    platSpec->socketFd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (platSpec->socketFd < 0) {
        FATAL_ERROR("Create socket failed");
    }
    struct sockaddr_un servAddr = { 0 };
    servAddr.sun_family = AF_UNIX;
    strcpy(servAddr.sun_path, socketName);
    if (connect(platSpec->socketFd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
        FATAL_ERROR("Couldn't connect");
    }

    // Read Xauthority.
    char xauthFilenameBuf[PATH_MAX];
    char const *xauthFilename = getenv("XAUTHORITY");
    if (!xauthFilename) {
        char const *homeDir = getenv("HOME");
        size_t homeDirLen = strlen(homeDir);
        char const xauth_appender[] = "/.Xauthority";
        size_t xauth_appender_len = sizeof(xauth_appender);
        if (homeDirLen + xauth_appender_len > PATH_MAX) {
            FATAL_ERROR("HOME too long");
        }
        memcpy(xauthFilenameBuf, homeDir, homeDirLen);
        strcpy(xauthFilenameBuf + homeDirLen, xauth_appender);
        xauthFilename = xauthFilenameBuf;
    }
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
    request.minor_version = 0;
    request.auth_proto_name_len = 18;
    request.auth_proto_data_len = 16;
    SendBuf(platSpec, &request, sizeof(connection_request_t));
    SendBuf(platSpec, "MIT-MAGIC-COOKIE-1\0\0", 20);
    SendBuf(platSpec, xauthCookie + xauthLen - 16, 16);

    // Read connection reply header.
    FatalRead(platSpec, &platSpec->connectionReplyHeader, sizeof(connectionReplyHeader_t));
    if (platSpec->connectionReplyHeader.success == 0) {
        FATAL_ERROR("Connection reply indicated failure.");
    }

    // Read rest of connection reply.
    platSpec->connectionReplySuccessBody = (connectionReplySuccessBody_t*)new char[platSpec->connectionReplyHeader.len * 4];
    FatalRead(platSpec, platSpec->connectionReplySuccessBody,
               platSpec->connectionReplyHeader.len * 4);

    // Set some pointers into the connection reply because they'll be convenient later.
    int vendorLenPlusPadding = (platSpec->connectionReplySuccessBody->vendor_len + 3) & ~3;
    platSpec->pixmapFormats = (pixmap_format_t *)(platSpec->connectionReplySuccessBody->vendor_string +
                             vendorLenPlusPadding);
    platSpec->screens = (screen_t *)(platSpec->pixmapFormats +
                                  platSpec->connectionReplySuccessBody->num_pixmapFormats);

    platSpec->nextResourceId = platSpec->connectionReplySuccessBody->id_base;

    // Get some Atom ids that we will use later.
    platSpec->clipboardId = GetAtomId(win, "CLIPBOARD");
    platSpec->stringId = GetAtomId(win, "STRING");
    platSpec->xselDataId = GetAtomId(win, "XSEL_DATA");
    platSpec->targetsId = GetAtomId(win, "TARGETS");
    platSpec->wmDeleteWindowId = GetAtomId(win, "WM_DELETE_WINDOW");
    platSpec->wmProtocolsId = GetAtomId(win, "WM_PROTOCOLS");
    printf("Atoms: clipboard=0x%x string=0x%x xsel=0x%x targets=0x%x wmDeleteWindow=0x%x "
        "wmProtocols=0x%x\n",
        platSpec->clipboardId, platSpec->stringId, platSpec->xselDataId, platSpec->targetsId,
        platSpec->wmDeleteWindowId, platSpec->wmProtocolsId);

    platSpec->windowId = generateId(win);
}


static void CreateGc(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    platSpec->graphicsContextId = generateId(win);
    int const len = 4;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_GC | (len<<16);
    packet[1] = platSpec->graphicsContextId;
    packet[2] = platSpec->windowId;
    packet[3] = 0; // Value mask.

    SendBuf(platSpec, packet, sizeof(packet));
}


DfWindow *CreateWin(int width, int height, WindowType winType, char const *winName) {
    return CreateWinPos(0, 0, width, height, winType, winName);
}


static void EnableDeleteWindowEvent(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;

    // See InterClient Communication Conventions Manual v2.0. The section
    // heading is "Window Deletion".
    int requestLen = 7;
    uint32_t packet[requestLen];
    packet[0] = X11_OPCODE_CHANGE_PROPERTY | (requestLen << 16);
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->wmProtocolsId; // Property.
    packet[3] = 4; // Type = ATOM.
    packet[4] = 32; // Format = 32 bits per item.
    packet[5] = 1; // Length = 1 item.
    packet[6] = platSpec->wmDeleteWindowId; // Item data.

    SendBuf(platSpec, packet, sizeof(packet));
}


static void MakeSocketNonBlocking(WindowPlatformSpecific *platSpec) {
    // Make socket non-blocking.
    int flags = fcntl(platSpec->socketFd, F_GETFL, 0);
    if (flags == -1) {
        FATAL_ERROR("Couldn't get flags of socket");
    }
    flags |= O_NONBLOCK;
    if (fcntl(platSpec->socketFd, F_SETFL, flags) != 0) {
        FATAL_ERROR("Couldn't set socket as non-blocking");
    }
}


DfWindow *CreateWinPos(int x, int y, int width, int height, WindowType windowed, char const *winName) {
    DfWindow *win = new DfWindow;
    memset(win, 0, sizeof(DfWindow));
    win->_private = new DfWindowPrivate;
    memset(win->_private, 0, sizeof(DfWindowPrivate));
    win->_private->platSpec = new WindowPlatformSpecific;
    memset(win->_private->platSpec, 0, sizeof(WindowPlatformSpecific));
    win->_private->platSpec->socketFd = -1;

    win->bmp = BitmapCreate(width, height);

    ConnectToXserver(win);

    WindowPlatformSpecific *platSpec = win->_private->platSpec;

    int const len = 9;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_WINDOW | (len<<16);
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->screens[0].root_id;
    packet[3] = 0; // x,y pos. System will position window. TODO - use x and y
    packet[4] = width | (height<<16);
    packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    packet[6] = 0; // Visual: Copy from parent.
    packet[7] = 0x800; // value_mask = event-mask
    packet[8] = X11_EVENT_KEYPRESS | X11_EVENT_KEYRELEASE | X11_EVENT_POINTERMOTION |
                X11_EVENT_BUTTONPRESS | X11_EVENT_BUTTONRELEASE | X11_EVENT_STRUCTURE_NOTIFY |
                X11_EVENT_FOCUSCHANGE;
    SendBuf(platSpec, packet, sizeof(packet));

    CreateGc(win);
    MapWindow(win);
    SetWindowTitle(win, winName);
    EnableDeleteWindowEvent(win);

    MakeSocketNonBlocking(platSpec);
    InitInput(win);

    return win;
}


void DestroyWin(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    uint32_t packet[2] = { 0 };
    packet[0] = 4; // OPCODE_DESTROY_WINDOW
    packet[0] |= 2 << 16; // Length
    packet[1] = platSpec->windowId;
    SendBuf(platSpec, packet, sizeof(packet));

    BitmapDelete(win->bmp);
    delete[] platSpec->connectionReplySuccessBody;
    delete win->_private->platSpec;
    delete win->_private;
    delete win;
}


static void BlitBitmapToWindow(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;

    // *** Send back-buffer to Xserver.
    int W = win->bmp->width;
    int H = win->bmp->height;
    enum { MAX_BYTES_PER_REQUEST = 65535 }; // Len field is 16-bits
    int num_rows_in_chunk = (MAX_BYTES_PER_REQUEST - 6) / W; // -6 because of packet header
    DfColour *row = win->bmp->pixels;
    for (int y = 0; y < H; y += num_rows_in_chunk) {
        if (y + num_rows_in_chunk > H) {
            num_rows_in_chunk = H - y;
        }

        uint32_t packet[6];
        uint32_t bmp_format = 2 << 8;
        uint32_t request_len = (uint32_t)(W * num_rows_in_chunk + 6) << 16;
        packet[0] = X11_OPCODE_PUT_IMAGE | bmp_format | request_len;
        packet[1] = platSpec->windowId;
        packet[2] = platSpec->graphicsContextId;
        packet[3] = W | (num_rows_in_chunk << 16); // Width and height.
        packet[4] = 0 | (y << 16); // Dst X and Y.
        packet[5] = 24 << 8; // Bit depth.

        SendBuf(platSpec, packet, sizeof(packet));
        SendBuf(platSpec, row, W * num_rows_in_chunk * 4);
        row += W * num_rows_in_chunk;
    }
}


static void CreateFakeWindowIfNeeded() {
    if (g_fakeWindow._private) return;

    g_fakeWindow._private = new DfWindowPrivate;
    memset(g_fakeWindow._private, 0, sizeof(DfWindowPrivate));
    g_fakeWindow._private->platSpec = new WindowPlatformSpecific;
    memset(g_fakeWindow._private->platSpec, 0, sizeof(WindowPlatformSpecific));
    g_fakeWindow._private->platSpec->socketFd = -1;
    ConnectToXserver(&g_fakeWindow);

    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;
    int const len = 9;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_WINDOW | (len<<16);
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->screens[0].root_id;
    packet[3] = 0; // x,y pos. System will position window. TODO - use x and y
    packet[4] = 1 | (1<<16); // width=1 height=1
    packet[5] = 0; // DEFAULT_BORDER and DEFAULT_GROUP.
    packet[6] = 0; // Visual: Copy from parent.
    packet[7] = 0x800; // value_mask = event-mask
    packet[8] = 0;
    SendBuf(platSpec, packet, sizeof(packet));

    MakeSocketNonBlocking(platSpec);
}


bool GetDesktopRes(int *width, int *height) {
    CreateFakeWindowIfNeeded();
    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;
    *width = platSpec->screens[0].width;
    *height = platSpec->screens[0].height;
    return true;
}


bool WaitVsync() {
    usleep(6667);
    return true;
}


bool InputPoll(DfWindow *win) {
    bool rv = HandleEvents(win);
    InputPollInternal(win);
    return rv;
}


void ShowMouse(DfWindow *win) {
}


void HideMouse(DfWindow *win) {
}


void SetMouseCursor(DfWindow *win, MouseCursorType t) {
}


bool IsWindowMaximized(DfWindow *win) {
    return false;
}


void SetMaximizedState(DfWindow *win, bool maximize) {
}


void BringWindowToFront(DfWindow *win) {
}


void SetWindowTitle(DfWindow *win, char const *title) {
    int headerNumBytes = 24;
    int titleLen = strlen(title);

    uint32_t packet[64] = { 0 };
    int maxTitleLen = sizeof(packet) - headerNumBytes;
    if (titleLen > maxTitleLen)
        titleLen = maxTitleLen;

    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int len = (headerNumBytes + titleLen + 3) / 4;
    packet[0] = X11_OPCODE_CHANGE_PROPERTY | (len << 16);
    packet[1] = platSpec->windowId;
    packet[2] = X11_ATOM_WM_NAME; // Property
    packet[3] = 0x1f; // Type
    packet[4] = 8; // Format
    packet[5] = titleLen;
    memcpy(&packet[6], title, titleLen);

    SendBuf(platSpec, packet, len * 4);
}


void SetWindowIcon(DfWindow *win) {}


static void SendGetPropertyRequest(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
//    puts("sending GetProperty request");
    uint32_t packet[6];
    packet[0] = 20 | 6 << 16;
    packet[1] = platSpec->windowId; // window
    packet[2] = platSpec->xselDataId;   // property
    packet[3] = 0; // Type = any
    packet[4] = 0; // offset = 0
    packet[5] = 0xfffffffful; // length
    SendBuf(platSpec, packet, sizeof(packet));
}


static void ReceiveClipboardData(DfWindow *win) {
    SendGetPropertyRequest(win);
//    printf("Getting GetProperty response\n");

    while (!GetReply(win, 32))
        ;

    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    uint8_t *buf = platSpec->recvBuf;
    uint32_t type = *((uint32_t *)(buf + 8));
    if (type == platSpec->stringId) {
        uint32_t replyLen = *((uint32_t *)(buf + 4)) * 4;

        uint32_t lenOfValueInFmtUnits = *((uint32_t *)(buf + 16));
//        printf("len of value in fmt units: %i\n", lenOfValueInFmtUnits);

        ConsumeMessage(platSpec, 32);

        platSpec->clipboardRxData = new char[lenOfValueInFmtUnits + 1];
        char *nextWritePoint = platSpec->clipboardRxData;

        uint32_t numBytesLeft = lenOfValueInFmtUnits;
        while (numBytesLeft > 0) {
            ReadFromXServer(platSpec);

            ssize_t stringLen = IntMin(platSpec->recvBufNumBytesAvailable, numBytesLeft);
            memcpy(nextWritePoint, (char const *)buf, stringLen);
            nextWritePoint += stringLen;

            ConsumeMessage(platSpec, stringLen);
            numBytesLeft -= stringLen;
        }
        platSpec->clipboardRxData[lenOfValueInFmtUnits] = '\0';

        uint32_t amtPadding = replyLen - lenOfValueInFmtUnits;
        ConsumeMessage(platSpec, amtPadding);
    }
}


char *X11InternalClipboardRequestData() {
    CreateFakeWindowIfNeeded();
    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;

    if (platSpec->clipboardRxData) return NULL;

//    puts("Sending ConvertSelection request");
    uint32_t packet[6];
    packet[0] = 24 | 6 << 16;
    packet[1] = platSpec->windowId; // requestor
    packet[2] = platSpec->clipboardId; // selection
    packet[3] = platSpec->stringId; // target
    packet[4] = platSpec->xselDataId; // property
    packet[5] = 0; // time
    SendBuf(platSpec, packet, sizeof(packet));

    double endTime = GetRealTime() + 0.1;
    do {
        InputPoll(&g_fakeWindow);

        if (platSpec->clipboardRxData) {
            break;
        }
    } while (GetRealTime() < endTime);

    return platSpec->clipboardRxData;
}


void X11InternalClipboardReleaseReceivedData() {
    CreateFakeWindowIfNeeded();
    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;
    delete [] platSpec->clipboardRxData;
    platSpec->clipboardRxData = NULL;
}


static void SendChangePropertyRequest(WindowPlatformSpecific *platSpec, uint32_t destWindow,
                                      uint32_t target, uint32_t property) {
    if (target == platSpec->targetsId) {
        // This branch sends a change property request that is used as the
        // response to a SelectionRequest. It tells the recipient what format the
        // clipboard data that we are about to send will be in.
//        puts("Sending ChangeProperty with clipboard data format");
        const int numWords = 8;
        uint32_t packet[numWords];
        packet[0] = X11_OPCODE_CHANGE_PROPERTY | numWords << 16;
        packet[1] = destWindow;
        packet[2] = property;
        packet[3] = 4; // type is ATOM.
        packet[4] = 32; // Format unit is 32 bits.
        packet[5] = 2; // Length is 2 format units;
        packet[6] = platSpec->targetsId;
        packet[7] = platSpec->stringId;
        SendBuf(platSpec, packet, sizeof(packet));
    }
    else {
        // This branch sends a change property request that includes the actual
        // clipboard data.
//        puts("Sending ChangeProperty with clipboard contents");

        const int amtPadding = 4 - platSpec->clipboardTxDataNumChars & 3;
        const int numWords = 6 + (platSpec->clipboardTxDataNumChars + amtPadding) / 4;
//        printf("amtPadding:%d numWords:%d numChars:%d\n", amtPadding, numWords, platSpec->clipboardTxDataNumChars);
        uint32_t packet[6];
        packet[0] = X11_OPCODE_CHANGE_PROPERTY | numWords << 16;
        packet[1] = destWindow;
        packet[2] = property;
        packet[3] = platSpec->stringId; // type is STRING.
        packet[4] = 8; // Format unit is this many bits.
        packet[5] = platSpec->clipboardTxDataNumChars; // Length in format units;
        SendBuf(platSpec, packet, sizeof(packet));
        SendBuf(platSpec, platSpec->clipboardTxData, platSpec->clipboardTxDataNumChars + amtPadding);
    }
}


static void SendSendEventSelectionNotify(WindowPlatformSpecific *platSpec, uint32_t destWindow,
                                         uint32_t target, uint32_t property, uint32_t time) {
//    puts("Sending SendEventSelectionNotify");
    uint32_t packet[11];
    packet[0] = 25 | 11 << 16;
    packet[1] = destWindow;
    packet[2] = 0; // event mask
    packet[3] = 31; // SelectionNotify
    packet[4] = time;
    packet[5] = destWindow; // requestor
    packet[6] = platSpec->clipboardId;
    packet[7] = target;
    packet[8] = property;
    packet[9] = 0;
    packet[10] = 0;

    SendBuf(platSpec, packet, sizeof(packet));
}


void X11InternalClipboardSetData(char const *data, int numChars) {
    CreateFakeWindowIfNeeded();
    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;

    delete [] platSpec->clipboardTxData;
    platSpec->clipboardTxData = new char[numChars + 3]; // +3 to allow for maximum amount of padding needed when buffer is sent to xServer.
    platSpec->clipboardTxDataNumChars = numChars;
    memcpy(platSpec->clipboardTxData, data, numChars);
    memset(platSpec->clipboardTxData + numChars, 0, 3); // Prevent Valgrind complaining about uninitialized memory.

//    puts("Sending SetSelectionOwner request");
    uint32_t packet[4];
    packet[0] = X11_OPCODE_SET_SELECTION_OWNER | 4 << 16;
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->clipboardId;
    packet[3] = 0;
    SendBuf(platSpec, packet, sizeof(packet));
}

// Platform includes
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/un.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>

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
//        DISPLAY=:1 ./CodeTrowel
//
// You will also see other X11 applications' traffic if they are launched with
// the same DISPLAY string.



//
// X11 protocol definitions

enum {
    X11_OPCODE_CREATE_WINDOW = 1,
    X11_OPCODE_CHANGE_WINDOW_ATTRIBUTES = 2,
    X11_OPCODE_MAP_WINDOW = 8,
    X11_OPCODE_CONFIGURE_WINDOW = 12,
    X11_OPCODE_INTERN_ATOM = 16,
    X11_OPCODE_CHANGE_PROPERTY = 18,
    X11_OPCODE_SET_SELECTION_OWNER = 22,
    X11_OPCODE_CONVERT_SELECTION = 24,
    X11_OPCODE_SEND_EVENT = 25,
    X11_OPCODE_CLIENT_MESSAGE = 33,
    X11_OPCODE_QUERY_KEYMAP = 44,
    X11_OPCODE_OPEN_FONT = 45,
    X11_OPCODE_CREATE_GC = 55,
    X11_OPCODE_PUT_IMAGE = 72,
    X11_OPCODE_CREATE_GLYPH_CURSOR = 94,
    X11_OPCODE_QUERY_EXTENSION = 98,

    X11_CW_EVENT_MASK = 1<<11,
    X11_EVENT_MASK_KEY_PRESS = 1,
    X11_EVENT_MASK_POINTER_MOTION = 1<<6,

    X11_ATOM_WM_NAME = 39
};

// MIT-SHM extension minor opcodes
enum {
    X11_SHM_ATTACH = 1,
    X11_SHM_DETACH = 2,
    X11_SHM_PUT_IMAGE = 3,
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
    X11_EVENT_FOCUSCHANGE = 0x200000,
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

    // Extensions
    uint8_t randrMajorOpcode;
    uint8_t shmMajorOpcode; // 0 if SHM not available

    // MIT-SHM resources
    int shmId;
    char *shmAddress;
    size_t shmSize;
    uint32_t shmSegId;

    // Atom values.
    int clipboardAtom;
    int stringAtom;
    int xselDataAtom;
    int targetsAtom;
    int wmDeleteWindowAtom;
    int wmProtocolsAtom;
    int xdndPositionAtom;
    int xdndStatusAtom;
    int xdndDropAtom;
    int xdndActionCopyAtom;
    int xdndSelectionAtom;
    int uriListAtom;
    int xdndFilenameAtom;
    int xdndFinishedAtom;
    int netMoveResizeWindowAtom;
    int netWmIconAtom;
    int cardinalAtom;

    int cursorIbeamId;
    int cursorResizeWidthId;
    int cursorResizeHeightId;
    int dropSourceWinId;

    MouseCursorType currentCursor;

    char *clipboardRxData;  // NULL except between calls of X11InternalClipboardRequestData() and ClipboardReleaseReceivedData()
    char *clipboardTxData;
    unsigned clipboardTxDataNumChars;

    int monitorX, monitorY, monitorW, monitorH, monitorDpi;
    bool monitorDetailsValid;
};


static bool GetReply(DfWindow *win, int expectedLen);
static void ReceiveClipboardData(DfWindow *win);
static void ReceiveDndData(DfWindow *win);


// Not a visible window. Just used to store a connection to the XServer. Used
// for things like GetDesktopRes() and the clipboard routines which need to
// even if not real window exists.
static DfWindow g_fakeWindow = { 0 };


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


static char DfKeycodeToAscii(unsigned char keycode, char modifiers) {
    int shift = modifiers & 1;
    int capsLock = modifiers & 2;
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
            return '\xac'; // ¬
        }
        return '`';
    }

    if (keycode >= KEY_0 && keycode <= KEY_9) {
        if (shift) {
            switch (keycode) {
               case KEY_0: return ')';
               case KEY_1: return '!';
               case KEY_2: return '"';
               case KEY_3: return '\xa3'; // £
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
        if (!shift && !capsLock) {
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
        if (errno == EAGAIN)
            return 0;

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
        if (sizeSent < 0)
            FATAL_ERROR("Couldn't send buf");

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


static uint32_t GetU32FromRecvBuf(WindowPlatformSpecific *platSpec, int offset) {
    return *((uint32_t*)(platSpec->recvBuf + offset));
}


// ****************************************************************************
// X11 protocol pack and send functions
// ****************************************************************************


static void MapWindow(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int const len = 2;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_MAP_WINDOW | (len<<16);
    packet[1] = platSpec->windowId;
    SendBuf(platSpec, packet, 8);
}


static void SendXdndStatus(DfWindow *win, int targetWinId) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int len = 11;
    uint32_t buf[len];
    buf[0] = X11_OPCODE_SEND_EVENT | (len << 16);
    buf[1] = targetWinId;
    buf[2] = 0;
    buf[3] = X11_OPCODE_CLIENT_MESSAGE;
    buf[3] |= (32 << 8); // format = 32-bit
    buf[4] = targetWinId;
    buf[5] = platSpec->xdndStatusAtom;
    buf[6] = platSpec->windowId;
    buf[7] = 1; // Accept drop messages flag.
    buf[8] = 0; // Min x & y values for drop area.
    buf[9] = 0; // Width and height of drop area.
    buf[10] = platSpec->xdndActionCopyAtom;
    SendBuf(platSpec, buf, sizeof(buf));
}


static void SendXConvertSelection(DfWindow *win, uint32_t timestamp) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int len = 6;
    uint32_t buf[len];
    buf[0] = X11_OPCODE_CONVERT_SELECTION | (len << 16);
    buf[1] = platSpec->windowId; // Requestor window, ie us.
    buf[2] = platSpec->xdndSelectionAtom; // We'd like the DND data please.
    buf[3] = platSpec->uriListAtom; // Specifically the file name
    buf[4] = platSpec->xdndFilenameAtom;
    buf[5] = timestamp;
    SendBuf(platSpec, buf, sizeof(buf));
}


static void SendGetPropertyRequest(DfWindow *win, uint32_t property) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    uint32_t packet[6];
    packet[0] = 20 | 6 << 16;
    packet[1] = platSpec->windowId; // window
    packet[2] = property;
    packet[3] = 0; // Type = any
    packet[4] = 0; // offset = 0
    packet[5] = 0xfffffffful; // length
    SendBuf(platSpec, packet, sizeof(packet));
}


static void SendXdndFinished(DfWindow *win, uint32_t targetWinId) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int len = 11;
    uint32_t buf[len] = { 0 };
    buf[0] = X11_OPCODE_SEND_EVENT | (len << 16);
    buf[1] = targetWinId;
    buf[2] = 0;
    buf[3] = X11_OPCODE_CLIENT_MESSAGE;
    buf[3] |= (32 << 8); // format = 32-bit
    buf[4] = targetWinId;
    buf[5] = platSpec->xdndFinishedAtom;
    buf[6] = platSpec->windowId;
    buf[7] = 1; // Flags: bit 0 = success
    buf[8] = platSpec->xdndActionCopyAtom;
    SendBuf(platSpec, buf, sizeof(buf));
}


static void SendChangePropertyRequest(WindowPlatformSpecific *platSpec, uint32_t destWindow,
                                      uint32_t target, uint32_t property) {
    if (target == platSpec->targetsAtom) {
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
        packet[6] = platSpec->targetsAtom;
        packet[7] = platSpec->stringAtom;
        SendBuf(platSpec, packet, sizeof(packet));
    }
    else {
        // This branch sends a change property request that includes the actual
        // clipboard data.
//        puts("Sending ChangeProperty with clipboard contents");

        const int amtPadding = (4 - (platSpec->clipboardTxDataNumChars & 3)) & 3;
        const int numWords = 6 + (platSpec->clipboardTxDataNumChars + amtPadding) / 4;
//        printf("amtPadding:%d numWords:%d numChars:%d\n", amtPadding, numWords, platSpec->clipboardTxDataNumChars);
        uint32_t packet[6];
        packet[0] = X11_OPCODE_CHANGE_PROPERTY | numWords << 16;
        packet[1] = destWindow;
        packet[2] = property;
        packet[3] = platSpec->stringAtom; // type is STRING.
        packet[4] = 8; // Format unit is this many bits.
        packet[5] = platSpec->clipboardTxDataNumChars; // Length in format units;
        SendBuf(platSpec, packet, sizeof(packet));
        SendBuf(platSpec, platSpec->clipboardTxData, platSpec->clipboardTxDataNumChars + amtPadding);
    }
}


static void SendEventSelectionNotify(WindowPlatformSpecific *platSpec, uint32_t destWindow,
                                         uint32_t target, uint32_t property, uint32_t time) {
    uint32_t packet[11];
    packet[0] = X11_OPCODE_SEND_EVENT | 11 << 16;
    packet[1] = destWindow;
    packet[2] = 0; // event mask
    packet[3] = 31; // SelectionNotify
    packet[4] = time;
    packet[5] = destWindow; // requestor
    packet[6] = platSpec->clipboardAtom;
    packet[7] = target;
    packet[8] = property;
    packet[9] = 0;
    packet[10] = 0;

    SendBuf(platSpec, packet, sizeof(packet));
}


static void SendQueryExtension(WindowPlatformSpecific *platSpec, char const *extensionString) {
    size_t stringLen = strlen(extensionString);
    size_t stringLenWithPadding = (stringLen + 3) & -4;
    size_t packetLen = 2 + stringLenWithPadding / 4;
    if (packetLen > 8)
        FATAL_ERROR("SendQueryExtension string too long");
    uint32_t packet[8] = { 0 };
    packet[0] = X11_OPCODE_QUERY_EXTENSION | (packetLen << 16);
    packet[1] = stringLen;
    memcpy(&packet[2], extensionString, stringLen);
    SendBuf(platSpec, packet, packetLen * 4);
}


static void SendRrGetMonitors(WindowPlatformSpecific *platSpec) {
    int len = 3;
    uint32_t packet[len];
    packet[0] = platSpec->randrMajorOpcode | (42 << 8) | (len << 16);
    packet[1] = platSpec->windowId;
    packet[2] = 1;
    SendBuf(platSpec, packet, sizeof(packet));
}


// ****************************************************************************
// Higher level X11 protocol handling
// ****************************************************************************

static uint32_t GenerateId(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    return platSpec->nextResourceId++;
}


static int CreateGlyphCursor(DfWindow *win, uint32_t fontId, uint16_t mainGlyphIdx) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    unsigned len = 8;
    uint32_t buf[len];
    uint16_t maskGlyphIdx = mainGlyphIdx + 1;
    int newCursorId = GenerateId(win);

    buf[0] = X11_OPCODE_CREATE_GLYPH_CURSOR | (len << 16);
    buf[1] = newCursorId;
    buf[2] = fontId;  // Source Font ID
    buf[3] = fontId;  // Mask Font ID (same font)
    buf[4] = mainGlyphIdx | (maskGlyphIdx << 16);
    buf[5] = 0; // Foreground red and green channels
    buf[6] = 0xffff0000ul; // Foreground blue in hi word, Background red in low.
    buf[7] = 0xfffffffful; // Background green and blue.

    SendBuf(platSpec, buf, sizeof(buf));
    return newCursorId;
}


static void OpenFont(DfWindow *win, uint32_t fontId, char const *fontName) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    uint16_t nameLen = (uint16_t)strlen(fontName);
    uint16_t padLen = (4 - (nameLen % 4)) % 4;

    // Request size in bytes: Header(4) + FontID(4) + NameLen(2) + Pad(2) + Name + Pad
    uint32_t totalBytes = 12 + nameLen + padLen;
    uint16_t wordCount = totalBytes / 4;

    uint32_t buf[256] = { 0 };
    buf[0] = X11_OPCODE_OPEN_FONT | (wordCount << 16);
    buf[1] = fontId;   // Client-allocated Font Resource ID
    buf[2] = nameLen;  // Length of font name string

    memcpy(&buf[3], fontName, nameLen);
    SendBuf(platSpec, buf, totalBytes);
}


static int GetAtomId(DfWindow *win, char const *atomName) {
    int atomNameLen = strlen(atomName);
    if (atomNameLen > 28) return 0;

    int requestLenWords = (11 + atomNameLen) / 4;
    int requestLenBytes = requestLenWords * 4;
    uint8_t packet[20 + 32] = { 0 };
    packet[0] = X11_OPCODE_INTERN_ATOM;
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


static int QueryExtension(DfWindow *win, char const *extensionString) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    SendQueryExtension(platSpec, extensionString);
    while (!GetReply(win, 32))
        ;
    uint8_t majorOpcode = platSpec->recvBuf[9];
    ConsumeMessage(platSpec, 32);
    return majorOpcode;
}


static void UpdateMonitorDetails(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    SendRrGetMonitors(platSpec);
    while (!GetReply(win, 32))
        ;

    // Extract the length field (bytes 4-7) which specifies how many 4-byte units follow the 32-byte header
    uint32_t length_units = GetU32FromRecvBuf(platSpec, 4);
    size_t total_reply_bytes = 32 + (length_units * 4);

    // 3. Ensure our non-blocking receive buffer has accumulated the *entire* reply payload
    while (platSpec->recvBufNumBytesAvailable < total_reply_bytes)
        ReadFromXServer(platSpec);

    uint32_t numMonitors = *(uint32_t *)(platSpec->recvBuf + 12);
    uint8_t *ptr = platSpec->recvBuf + 32;
    for (uint32_t i = 0; i < numMonitors; i++) {
        // Skip 'name' (4 bytes) + 'primary' (1) + 'automatic' (1) = 6 bytes to get to noutput
        uint16_t n_output = *(uint16_t *)(ptr + 6);

        platSpec->monitorX = *(int16_t *)(ptr + 8);
        platSpec->monitorY = *(int16_t *)(ptr + 10);
        platSpec->monitorW = *(uint16_t *)(ptr + 12);
        platSpec->monitorH = *(uint16_t *)(ptr + 14);
        uint32_t mwidth = *(uint32_t *)(ptr + 16);
        platSpec->monitorDpi = 96;
        if (mwidth > 0)
            platSpec->monitorDpi = (platSpec->monitorW * 25.4) / mwidth;

        int right = platSpec->monitorX + platSpec->monitorW;
        int bottom = platSpec->monitorY + platSpec->monitorH;
        if (win->left >= platSpec->monitorX && win->left < right &&
                win->top >= platSpec->monitorY && win->top < bottom) {
            platSpec->monitorDetailsValid = true;
            break;
        }

        // Advance pointer past this monitor block:
        // 24 fixed bytes + (n_output * 4 bytes for the output Atom/RROutput IDs)
        size_t block_size = 24 + (n_output * sizeof(uint32_t));

        // X11 protocol alignment requirement: pad each monitor block to a multiple of 4 bytes
        size_t remainder = block_size % 4;
        if (remainder != 0)
            block_size += (4 - remainder);

        ptr += block_size;
    }

    ConsumeMessage(platSpec, total_reply_bytes);
}


static void InitShmSegment(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (platSpec->shmMajorOpcode == 0) return;

    size_t requiredSize = win->bmp->width * win->bmp->height * sizeof(DfColour);
    if (requiredSize <= platSpec->shmSize) return;

    // Clean up old segment if resizing.
    if (platSpec->shmId != -1) {
        // Send ShmDetach request to X server
        uint32_t detachPacket[2];
        detachPacket[0] = platSpec->shmMajorOpcode | (X11_SHM_DETACH << 8) | (2 << 16);
        detachPacket[1] = platSpec->shmSegId;
        SendBuf(platSpec, detachPacket, sizeof(detachPacket));

        shmdt(platSpec->shmAddress);
        shmctl(platSpec->shmId, IPC_RMID, NULL);
        platSpec->shmId = -1;
    }


    // Allocate System V Shared Memory.
    platSpec->shmSize = requiredSize * 1.3;
    platSpec->shmId = shmget(IPC_PRIVATE, platSpec->shmSize, IPC_CREAT | 0600);
    if (platSpec->shmId < 0) {
        platSpec->shmMajorOpcode = 0;
        return;
    }

    platSpec->shmAddress = (char *)shmat(platSpec->shmId, NULL, 0);
    if (platSpec->shmAddress == (char *)-1) {
        shmctl(platSpec->shmId, IPC_RMID, NULL);
        platSpec->shmId = -1;
        platSpec->shmMajorOpcode = 0;
        return;
    }

    // Generate an X11 Resource ID for the shared memory segment
    platSpec->shmSegId = GenerateId(win);

    // Send ShmAttach request to X server
    uint32_t attachPacket[4];
    attachPacket[0] = platSpec->shmMajorOpcode | (X11_SHM_ATTACH << 8) | (4 << 16);
    attachPacket[1] = platSpec->shmSegId;
    attachPacket[2] = platSpec->shmId;
    attachPacket[3] = 0; // Readonly = false (0)
    SendBuf(platSpec, attachPacket, sizeof(attachPacket));
}


static void TranslateCoordinates(DfWindow *win, int *x, int *y) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int len = 4;
    uint32_t packet[len];
    packet[0] = 40 /* X_TranslateCoordinates */ | (len << 16);
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->screens[0].root_id;
    packet[3] = *x | (*y << 16);
    SendBuf(platSpec, packet, sizeof(packet));

    while (!GetReply(win, 32))
        ;

    *x = *(int16_t *)(platSpec->recvBuf + 12);
    *y = *(int16_t *)(platSpec->recvBuf + 14);
    ConsumeMessage(platSpec, 32);
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
    if (platSpec->socketFd < 0)
        FATAL_ERROR("Create socket failed");
    struct sockaddr_un servAddr = { 0 };
    servAddr.sun_family = AF_UNIX;
    strcpy(servAddr.sun_path, socketName);
    if (connect(platSpec->socketFd, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        FATAL_ERROR("Couldn't connect");

    // Read Xauthority.
    char xauthFilenameBuf[2048];
    char const *xauthFilename = getenv("XAUTHORITY");
    if (!xauthFilename) {
        char const *homeDir = getenv("HOME");
        size_t homeDirLen = strlen(homeDir);
        char const xauthAppender[] = "/.Xauthority";
        size_t xauthAppenderLen = sizeof(xauthAppender);
        if (homeDirLen + xauthAppenderLen >= sizeof(xauthFilenameBuf)) {
            FATAL_ERROR("HOME too long");
        }
        memcpy(xauthFilenameBuf, homeDir, homeDirLen);
        strcpy(xauthFilenameBuf + homeDirLen, xauthAppender);
        xauthFilename = xauthFilenameBuf;
    }
    FILE *xauthFile = fopen(xauthFilename, "rb");
    if (!xauthFile)
        FATAL_ERROR("Couldn't open '%s'.", xauthFilename);
    char xauthCookie[4096];
    size_t xauthLen = fread(xauthCookie, 1, sizeof(xauthCookie), xauthFile);
    if (xauthLen < 0)
        FATAL_ERROR("Couldn't read from .Xauthority.");
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
    if (platSpec->connectionReplyHeader.success == 0)
        FATAL_ERROR("Connection reply indicated failure.");

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
    platSpec->clipboardAtom = GetAtomId(win, "CLIPBOARD");
    platSpec->stringAtom = GetAtomId(win, "STRING");
    platSpec->xselDataAtom = GetAtomId(win, "XSEL_DATA");
    platSpec->targetsAtom = GetAtomId(win, "TARGETS");
    platSpec->wmDeleteWindowAtom = GetAtomId(win, "WM_DELETE_WINDOW");
    platSpec->wmProtocolsAtom = GetAtomId(win, "WM_PROTOCOLS");
    platSpec->xdndPositionAtom = GetAtomId(win, "XdndPosition");
    platSpec->xdndStatusAtom = GetAtomId(win, "XdndStatus");
    platSpec->xdndDropAtom = GetAtomId(win, "XdndDrop");
    platSpec->xdndActionCopyAtom = GetAtomId(win, "XdndActionCopy");
    platSpec->xdndSelectionAtom = GetAtomId(win, "XdndSelection");
    platSpec->uriListAtom = GetAtomId(win, "text/uri-list");
    platSpec->xdndFilenameAtom = GetAtomId(win, "XdndFilename");
    platSpec->xdndFinishedAtom = GetAtomId(win, "XdndFinished");
    platSpec->netMoveResizeWindowAtom = GetAtomId(win, "_NET_MOVERESIZE_WINDOW");
    platSpec->netWmIconAtom = GetAtomId(win, "_NET_WM_ICON");
    platSpec->cardinalAtom = GetAtomId(win, "CARDINAL");

    platSpec->windowId = GenerateId(win);

    // Setup mouse cursors
    uint32_t fontId = GenerateId(win);
    OpenFont(win, fontId, "cursor");
    platSpec->cursorIbeamId = CreateGlyphCursor(win, fontId, 152);
    platSpec->cursorResizeWidthId = CreateGlyphCursor(win, fontId, 108);
    platSpec->cursorResizeHeightId = CreateGlyphCursor(win, fontId, 116);
    platSpec->currentCursor = MCT_ARROW;

    // Query extensions
    platSpec->randrMajorOpcode = QueryExtension(win, "RANDR");
    platSpec->shmMajorOpcode = QueryExtension(win, "MIT-SHM");
    platSpec->shmId = -1;
}


static void CreateGc(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    platSpec->graphicsContextId = GenerateId(win);
    int const len = 4;
    uint32_t packet[len];
    packet[0] = X11_OPCODE_CREATE_GC | (len<<16);
    packet[1] = platSpec->graphicsContextId;
    packet[2] = platSpec->windowId;
    packet[3] = 0; // Value mask.

    SendBuf(platSpec, packet, sizeof(packet));
}


static void EnableDeleteWindowEvent(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;

    // See InterClient Communication Conventions Manual v2.0. The section
    // heading is "Window Deletion".
    int requestLen = 7;
    uint32_t packet[requestLen];
    packet[0] = X11_OPCODE_CHANGE_PROPERTY | (requestLen << 16);
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->wmProtocolsAtom; // Property.
    packet[3] = 4; // Type = ATOM.
    packet[4] = 32; // Format = 32 bits per item.
    packet[5] = 1; // Length = 1 item.
    packet[6] = platSpec->wmDeleteWindowAtom; // Item data.

    SendBuf(platSpec, packet, sizeof(packet));
}


static void MakeSocketNonBlocking(WindowPlatformSpecific *platSpec) {
    // Make socket non-blocking.
    int flags = fcntl(platSpec->socketFd, F_GETFL, 0);
    if (flags == -1)
        FATAL_ERROR("Couldn't get flags of socket");
    flags |= O_NONBLOCK;
    if (fcntl(platSpec->socketFd, F_SETFL, flags) != 0) {
        FATAL_ERROR("Couldn't set socket as non-blocking");
    }
}


static void BlitBitmapToWindow(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int w = win->bmp->width;
    int h = win->bmp->height;

    // Fall back to standard chunks if SHM failed to initialize.
    if (platSpec->shmMajorOpcode == 0 || platSpec->shmId == -1) {
        // *** Send back-buffer to Xserver via standard chunks (your old code) ***
        enum { MAX_BYTES_PER_REQUEST = 65535 };
        int numRowsInChunk = (MAX_BYTES_PER_REQUEST - 6) / w;
        DfColour *row = win->bmp->pixels;
        for (int y = 0; y < h; y += numRowsInChunk) {
            if (y + numRowsInChunk > h) numRowsInChunk = h - y;

            uint32_t packet[6];
            uint32_t bmpFormat = 2 << 8;
            uint32_t requestLen = (uint32_t)(w * numRowsInChunk + 6) << 16;
            packet[0] = X11_OPCODE_PUT_IMAGE | bmpFormat | requestLen;
            packet[1] = platSpec->windowId;
            packet[2] = platSpec->graphicsContextId;
            packet[3] = w | (numRowsInChunk << 16);
            packet[4] = 0 | (y << 16);
            packet[5] = 24 << 8;

            SendBuf(platSpec, packet, sizeof(packet));
            SendBuf(platSpec, row, w * numRowsInChunk * 4);
            row += w * numRowsInChunk;
        }
        return;
    }

    // --- MIT-SHM Fast Path ---
    // Copy your pixel back-buffer straight into shared memory.
    memcpy(platSpec->shmAddress, win->bmp->pixels, w * h * sizeof(DfColour));

    // Send a single lightweight ShmPutImage request over the socket.
    uint32_t packet[10];
    packet[0] = platSpec->shmMajorOpcode | (X11_SHM_PUT_IMAGE << 8) | (10 << 16);
    packet[1] = platSpec->windowId;
    packet[2] = platSpec->graphicsContextId;
    packet[3] = w | (h << 16);                // total_width, total_height
    packet[4] = 0;                            // src_x, src_y
    packet[5] = w | (h << 16);
    packet[6] = 0;                            // dst_x, dst_y
    packet[7] = 24 | (2 << 8) | (0 << 16);    // depth (8), format ZPixmap=2 (8), send_event=0 (8), pad (8)
    packet[8] = platSpec->shmSegId;
    packet[9] = 0;                            // Offset in shared memory

    SendBuf(platSpec, packet, sizeof(packet));
}


static void HandleEvent(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;

    uint32_t selectionNotifyProperty = 0;
    bool winPosTranslateNeeded = false;
    bool redrawNeeded = false;

    switch (platSpec->recvBuf[0] & 0x7f) {
    case 0: // Error report.
        FATAL_ERROR("Received ERROR report, code %d\n", platSpec->recvBuf[1]);
        break;

    case 2: // KeyPress event.
        {
            unsigned char x11Keycode = platSpec->recvBuf[1];
            unsigned char dfKeycode = x11KeycodeToDfKeycode(x11Keycode);
            win->input.keyDowns[dfKeycode] = 1;
            win->input.keys[dfKeycode] = 1;
            int modifiers = platSpec->recvBuf[28];
            char ascii = DfKeycodeToAscii(dfKeycode, modifiers);
    //printf("Key down. x11Keycode:%i. dfKeycode:%i. Ascii:%c. Modifiers: 0x%x\n", x11Keycode, dfKeycode, ascii, modifiers);

            if (ascii) {
                win->input.keysTyped[win->input.numKeysTyped] = ascii;
                win->input.numKeysTyped++;
            }
            break;
        }

    case 3: // KeyRelease event.
        {
            unsigned char x11Keycode = platSpec->recvBuf[1];
            unsigned char dfKeycode = x11KeycodeToDfKeycode(x11Keycode);
            win->input.keyUps[dfKeycode] = 1;
            win->input.keys[dfKeycode] = 0;
    //printf("Key up. x11Keycode:%i. dfKeycode:%i.\n", x11Keycode, dfKeycode);
            break;
        }

    case 4: // Mouse down button event (includes scroll motion).
        switch (platSpec->recvBuf[1]) {
            case 1:
                win->input.lmb = 1;
                win->input.lmbClicked = true;
                break;
            case 2:
                win->input.mmb = 1;
                win->input.mmbClicked = true;
                break;
            case 3:
                win->input.rmb = 1;
                win->input.rmbClicked = true;
                break;
            case 4: win->input.mouseZ += 120; break;
            case 5: win->input.mouseZ -= 120; break;
        }
        //printf("down:%i\n", platSpec->recvBuf[1]);
        break;

    case 5: // Mouse button up event.
        switch (platSpec->recvBuf[1]) {
            case 1:
                win->input.lmb = 0;
                win->input.lmbUnClicked = true;
                break;
            case 2:
                win->input.mmb = 0;
                win->input.mmbUnClicked = true;
                break;
            case 3:
                win->input.rmb = 0;
                win->input.rmbUnClicked = true;
                break;
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
        HandleFocusOutEvent(win);
        break;

    case 22: // Configure notify event.
        {
            win->left = platSpec->recvBuf[16] + (platSpec->recvBuf[17] << 8);
            win->top = platSpec->recvBuf[18] + (platSpec->recvBuf[19] << 8);
            platSpec->monitorDetailsValid = false;

            if (!(platSpec->recvBuf[0] & 0x80))
                winPosTranslateNeeded = true;
            int w = platSpec->recvBuf[20] + (platSpec->recvBuf[21] << 8);
            int h = platSpec->recvBuf[22] + (platSpec->recvBuf[23] << 8);
            if (w != win->bmp->width || h != win->bmp->height) {
                BitmapDelete(win->bmp);
                win->bmp = BitmapCreate(w, h);
                InitShmSegment(win);
                redrawNeeded = true;
            }
        }
        break;

    case 30: // Selection Request
        {
            uint32_t time = GetU32FromRecvBuf(platSpec, 4);
            uint32_t requestor = GetU32FromRecvBuf(platSpec, 12);
            uint32_t selection = GetU32FromRecvBuf(platSpec, 16);
            ReleaseAssert(selection == platSpec->clipboardAtom, "selection was %x", selection);
            uint32_t target = GetU32FromRecvBuf(platSpec, 20);
            uint32_t property = GetU32FromRecvBuf(platSpec, 24);
//             printf("Recv'd Selection request. Time=%x Requestor=%x target=%x property=%x\n",
//                 time, requestor, target, property);

            SendChangePropertyRequest(platSpec, requestor, target, property);
            SendEventSelectionNotify(platSpec, requestor, target, property, time);
        }
        break;

    case 31: // Selection Notify
        // This event is received either as part of a Drag and Drop sequence or
        // in a get-data-from-clipboard sequence. It is a response to a
        // ConvertSelection request we sent to the server.
        selectionNotifyProperty = GetU32FromRecvBuf(platSpec, 20);
//        printf("Got selection notify 0x%x\n", selectionNotifyProperty);
        break;

    case 33: // Client message.
        {
            int type = GetU32FromRecvBuf(platSpec, 8);
            if (type == platSpec->wmProtocolsAtom) {
                win->windowClosed = true;
            }
            else if (type == platSpec->xdndPositionAtom) {
                int winToSendTo = GetU32FromRecvBuf(platSpec, 12);
                printf("xdndPosition from win %x\n", winToSendTo);
                SendXdndStatus(win, winToSendTo);
            }
            else if (type == platSpec->xdndDropAtom) {
                uint32_t timestamp = GetU32FromRecvBuf(platSpec, 24);
                platSpec->dropSourceWinId = GetU32FromRecvBuf(platSpec, 12);
                printf("xdndDrop timestamp=%x srcWinId=%x\n", timestamp, platSpec->dropSourceWinId);
                SendXConvertSelection(win, timestamp);
            }
            else {
                printf("Got ClientMessage. type=0x%x\n", type);
            }
        }
        break;

    default:
        printf("Got an unknown message type (%d).\n", platSpec->recvBuf[0] & 0x7f);
    }

    ConsumeMessage(platSpec, 32);

    if (winPosTranslateNeeded)
        TranslateCoordinates(win, &win->left, &win->top);

    if (selectionNotifyProperty == platSpec->xselDataAtom)
        ReceiveClipboardData(win);
    else if (selectionNotifyProperty == platSpec->xdndFilenameAtom)
        ReceiveDndData(win);

    if (redrawNeeded && win->redrawCallback) {
        win->redrawCallback();
    }
}


static bool HandleEvents(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    ReadFromXServer(platSpec);

    bool rv = false;
    while (platSpec->recvBufNumBytesAvailable >= 32 && platSpec->recvBuf[0] != 1) {
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
// Clipboard functions
// ****************************************************************************

static void *FakeWindowMessageLoop(void *ignored) {
    // This event loop only exists to handle get-clipboard-data requests. We'd
    // like to only poll for these events infrequently to minimised CPU usage.
    // But, perhaps surprisingly, the paste action in another app results in
    // about ten events being sent to us. We need to respond to all of them
    // quickly enough so that paste in the other application doesn't feel laggy.
    // As a result, the sleep behaviour in this loop is more complex than you'd
    // think it needs to be.
    int numItersUntilSlow = 0;
    while (1) {
        if (HandleEvents(&g_fakeWindow))
            numItersUntilSlow = 100;

        if (numItersUntilSlow < 1) {
            SleepMillisec(100);
            numItersUntilSlow = -1;
        }
        else {
            SleepMillisec(1);
            numItersUntilSlow--;
        }
    }

    return NULL;
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

    pthread_t thread;
    pthread_create(&thread, NULL, FakeWindowMessageLoop, NULL);
}


static void ReceiveClipboardData(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    SendGetPropertyRequest(win, platSpec->xselDataAtom);
//    printf("Getting GetProperty response\n");

    while (!GetReply(win, 32))
        ;

    uint8_t *buf = platSpec->recvBuf;
    uint32_t type = *((uint32_t *)(buf + 8));
    if (type == platSpec->stringAtom) {
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


static void ReceiveDndData(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    SendGetPropertyRequest(win, platSpec->xdndFilenameAtom);

    while (!GetReply(win, 32))
        ;

    uint8_t *buf = platSpec->recvBuf;
    uint32_t replyLen = *((uint32_t *)(buf + 4)) * 4;
    uint32_t lenOfValueInFmtUnits = *((uint32_t *)(buf + 16));
    ConsumeMessage(platSpec, 32);
    char *dndData = new char[lenOfValueInFmtUnits + 1];
    char *nextWritePoint = dndData;

    uint32_t numBytesLeft = lenOfValueInFmtUnits;
    while (numBytesLeft > 0) {
        ReadFromXServer(platSpec);

        ssize_t stringLen = IntMin(platSpec->recvBufNumBytesAvailable, numBytesLeft);
        memcpy(nextWritePoint, (char const *)buf, stringLen);
        nextWritePoint += stringLen;

        ConsumeMessage(platSpec, stringLen);
        numBytesLeft -= stringLen;
    }
    dndData[lenOfValueInFmtUnits] = '\0';

    uint32_t amtPadding = replyLen - lenOfValueInFmtUnits;
    ConsumeMessage(platSpec, amtPadding);

    SendXdndFinished(win, platSpec->dropSourceWinId);
    if (lenOfValueInFmtUnits > 7 && memcmp(dndData, "file://", 7) == 0)
        win->fileDropCallback(dndData + 7);
    delete[] dndData;
}


char *X11InternalClipboardRequestData() {
    CreateFakeWindowIfNeeded();
    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;

    if (platSpec->clipboardRxData) return NULL;

//    puts("Sending ConvertSelection request");
    uint32_t packet[6];
    packet[0] = 24 | 6 << 16;
    packet[1] = platSpec->windowId; // requestor
    packet[2] = platSpec->clipboardAtom; // selection
    packet[3] = platSpec->stringAtom; // target
    packet[4] = platSpec->xselDataAtom; // property
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
    packet[2] = platSpec->clipboardAtom;
    packet[3] = 0;
    SendBuf(platSpec, packet, sizeof(packet));
}


// ****************************************************************************
// Deadfrog Lib API functions
// ****************************************************************************

bool GetDesktopRes(int *width, int *height) {
    CreateFakeWindowIfNeeded();
    WindowPlatformSpecific *platSpec = g_fakeWindow._private->platSpec;
    *width = platSpec->screens[0].width;
    *height = platSpec->screens[0].height;
    return true;
}


DfWindow *CreateWin(int width, int height, WindowType winType, char const *winName) {
    return CreateWinPos(0, 0, width, height, winType, winName);
}


bool WaitVsync() {
    usleep(6667);
    return true;
}


bool InputPoll(DfWindow *win) {
    PreInputPollInternal(win);

    bool rv = HandleEvents(win);
    InputPollInternal(win);
    return rv;
}


void ShowMouse(DfWindow *win) {
}


void HideMouse(DfWindow *win) {
}


void SetMouseCursor(DfWindow *win, MouseCursorType t) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (t == platSpec->currentCursor) return;

    int len = 4;
    uint32_t buf[len];

    buf[0] = X11_OPCODE_CHANGE_WINDOW_ATTRIBUTES | (len << 16);
    buf[1] = platSpec->windowId;
    buf[2] = 0x4000; // CW_CURSOR

    switch (t) {
    case MCT_IBEAM: buf[3] = platSpec->cursorIbeamId; break;
    case MCT_RESIZE_WIDTH: buf[3] = platSpec->cursorResizeWidthId; break;
    case MCT_RESIZE_HEIGHT: buf[3] = platSpec->cursorResizeHeightId; break;
    default: buf[3] = 0; break;
    }

    SendBuf(platSpec, buf, sizeof(buf));
    platSpec->currentCursor = t;
}


void SetWindowIcon(DfWindow *win, int width, int height, DfColour *pixels) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;

    uint32_t numPixels = width * height;
    // Total request length in 32-bit words: Header (6) + Dims (2) + Pixels (numPixels)
    uint32_t totalWords = 6 + 2 + numPixels;

    uint32_t buf[8];
    buf[0] = X11_OPCODE_CHANGE_PROPERTY | (totalWords << 16);
    buf[1] = platSpec->windowId;
    buf[2] = platSpec->netWmIconAtom;
    buf[3] = platSpec->cardinalAtom;
    buf[4] = 32;            // Format: 32 bits per item
    buf[5] = 2 + numPixels; // Data length in 32-bit format units (width + height + pixels)
    buf[6] = width;
    buf[7] = height;
    SendBuf(platSpec, buf, sizeof(buf));

    SendBuf(platSpec, pixels, numPixels * sizeof(uint32_t));
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

    InitShmSegment(win);

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

    if (platSpec->shmId != -1) {
        shmdt(platSpec->shmAddress);
        shmctl(platSpec->shmId, IPC_RMID, NULL);
    }

    BitmapDelete(win->bmp);
    delete[] platSpec->connectionReplySuccessBody;
    delete win->_private->platSpec;
    delete win->_private;
    delete win;
}


int GetMonitorDpi(DfWindow *win) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (!platSpec->monitorDetailsValid)
        UpdateMonitorDetails(win);
    return platSpec->monitorDpi;
}


void GetMonitorWorkArea(DfWindow *win, int *x, int *y, int *w, int *h) {
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    if (!platSpec->monitorDetailsValid)
        UpdateMonitorDetails(win);
    *x = platSpec->monitorX;
    *y = platSpec->monitorY;
    *w = platSpec->monitorW;
    *h = platSpec->monitorH;
}


void SetWindowRect(DfWindow *win, int x, int y, int width, int height) {
    printf("swr %d %d %d %d\n", x, y, width, height);
    WindowPlatformSpecific *platSpec = win->_private->platSpec;
    int len = 11;
    uint32_t buf[len];
    buf[0] = X11_OPCODE_SEND_EVENT | (len << 16);
    buf[1] = platSpec->screens[0].root_id;       // Destination: Root window
    buf[2] = 0x80000 | 0x10000;                  // Event Mask: SubstructureRedirectMask | SubstructureNotifyMask
    buf[3] = X11_OPCODE_CLIENT_MESSAGE | (32 << 8); // Format: 32-bit client message
    buf[4] = platSpec->windowId;                 // Target window being moved/resized
    buf[5] = platSpec->netMoveResizeWindowAtom;  // Atom: _NET_MOVERESIZE_WINDOW
    buf[6] = 0x1f00;                             // Flags: x, y, width, and height value flags are set
    buf[7] = x;
    buf[8] = y;
    buf[9] = width;
    buf[10] = height;

    SendBuf(platSpec, buf, sizeof(buf));
}

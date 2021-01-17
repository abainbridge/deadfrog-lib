// This module implements window creation and mouse and keyboard input.

// You can test if any specific key has been pressed this frame using
// g_input.keyDowns[KEY_ESC].

// You can see how much the mouse has moved along the X axis this frame using
// g_input.mouseVelX.

// It also creates a bitmap the same size as the window to use as the back
// buffer of a double buffer system.

#pragma once


#include "df_bitmap.h"
#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define MAX_KEYS_TYPED_PER_FRAME 16
#define KEY_MAX 256

typedef void (RedrawCallback)(void);


typedef struct
{
    bool		windowHasFocus;
    bool        eventSinceAdvance;  // True if we've seen any events since the last advance

    bool		lmb;				// Mouse button states now. These can change in the
	bool		mmb;				// middle of the frame.
    bool		rmb;

	bool		lmbDoubleClicked;

    bool		lmbClicked;			// Mouse went from up to down this frame
	bool		mmbClicked;
    bool		rmbClicked;

    bool		lmbUnClicked;		// Mouse went from down to up this frame
	bool		mmbUnClicked;
    bool		rmbUnClicked;

    int			mouseX;				// Mouse pos now. Can changes through the course of the frame
    int			mouseY;
    int			mouseZ;

    int			mouseVelX;
    int			mouseVelY;
    int			mouseVelZ;

    // These variables store the key presses that have occurred this frame
	char		keysTyped[MAX_KEYS_TYPED_PER_FRAME];
	int			numKeysTyped;

    int         numKeyDowns;
    int         numKeyUps;
    signed char keys[KEY_MAX];		// Is the key currently pressed
    signed char keyDowns[KEY_MAX];	// Was the key pressed this frame (includes key repeats)
    signed char keyUps[KEY_MAX];	// Was the key released this frame
} DfInput;


typedef struct _DfWindow
{
    DfBitmap            *bmp;

    // Position of the window on the desktop. This is the top left of the
    // non-client rectangle. The main use is to allow your app to restore its
    // window to the same position is was at on the previous run. eg store
    // top,left in a config file before your app is closed, load them back in
	// next time the app starts and pass them to CreateWinPos().
    int                 left, top;

    bool                windowClosed;
    unsigned int	    fps;
    double              advanceTime; // Time between last two calls of UpdateWin().
    RedrawCallback      *redrawCallback;
    // TODO - add an isMinimized flag and use it where you see if (g_window->bmp->width < 100)
} DfWindow;


typedef enum
{
    WT_FULLSCREEN = 0,
    WT_WINDOWED = 1
} WindowType;


DLL_API DfWindow *g_window;
DLL_API DfInput g_input;

DLL_API bool GetDesktopRes(int *width, int *height);

// Creates a Window (fullscreen is really just a big window) and a bitmap the size of the window
// to use as the back buffer of a double buffer.
DLL_API bool CreateWin(int width, int height, WindowType windowed, char const *winName);
DLL_API bool CreateWinPos(int x, int y, int width, int height, WindowType windowed, char const *winName);

// Blit back buffer to screen and update FPS counter.
DLL_API void UpdateWin();

DLL_API void ShowMouse();
DLL_API void HideMouse();

DLL_API bool WaitVsync();   // Returns false if not supported.

// Register a callback that will be called when the window is resized. This gives the
// app a chance to redraw the window contents immediately. Without this, when the
// window is enlarged, the new part of the window will appear blank until the resize
// drag ends. This can also be used to resize any window sized bitmaps you have. The
// one in the DfWindow struct will have been resized by the time the callback is
// called.
DLL_API void RegisterRedrawCallback(RedrawCallback *proc);


DLL_API char const *GetKeyName(int i);
DLL_API int	        GetKeyId(char const *name);
DLL_API bool	    InputPoll();  // Returns true if any events occurred since last call


// Defines for indexes into g_input.keys[], keyDowns[] and keyUps[].
enum
{
    KEY_BACKSPACE = 8,
    KEY_TAB = 9,
    KEY_ENTER = 13,

    KEY_SHIFT = 16,
    KEY_CONTROL = 17,
    KEY_ALT = 18,
    KEY_PAUSE = 19,
    KEY_CAPSLOCK = 20,
    KEY_ESC = 27,

    KEY_SPACE = 32,

    KEY_PGUP = 33,
    KEY_PGDN = 34,
    KEY_END = 35,
    KEY_HOME = 36,
    KEY_LEFT = 37,
    KEY_RIGHT = 39,
    KEY_UP = 38,
    KEY_DOWN = 40,
    KEY_INSERT = 45,
    KEY_DEL = 46,

    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,

    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,

    KEY_MENU = 93,

    KEY_0_PAD = 96,
    KEY_1_PAD = 97,
    KEY_2_PAD = 98,
    KEY_3_PAD = 99,
    KEY_4_PAD = 100,
    KEY_5_PAD = 101,
    KEY_6_PAD = 102,
    KEY_7_PAD = 103,
    KEY_8_PAD = 104,
    KEY_9_PAD = 105,

    KEY_ASTERISK = 106,
    KEY_PLUS_PAD = 107,
    KEY_MINUS_PAD = 109,
    KEY_DEL_PAD = 110,
    KEY_SLASH_PAD = 111,

    KEY_F1 = 112,
    KEY_F2 = 113,
    KEY_F3 = 114,
    KEY_F4 = 115,
    KEY_F5 = 116,
    KEY_F6 = 117,
    KEY_F7 = 118,
    KEY_F8 = 119,
    KEY_F9 = 120,
    KEY_F10 = 121,
    KEY_F11 = 122,
    KEY_F12 = 123,

    KEY_NUMLOCK = 144,
    KEY_SCRLOCK = 145,

    KEY_COLON = 186,
    KEY_EQUALS = 187,
    KEY_COMMA = 188,
    KEY_MINUS = 189,
    KEY_STOP = 190,
    KEY_SLASH = 191,
    KEY_QUOTE = 192,

    KEY_OPENBRACE = 219,
    KEY_BACKSLASH = 220,
    KEY_CLOSEBRACE = 221,

    KEY_TILDE = 222,
    KEY_BACK_TICK = 223
};


#ifdef __cplusplus
}
#endif

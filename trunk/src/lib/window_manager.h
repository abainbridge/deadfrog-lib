// This module implements code to open a window. It connects the created window
// to the input system (input.h) so that mouse and keyboard events are 
// captured. It also creates a bitmap the same size as the window to use as
// the back buffer of a double buffer system.

#ifndef INCLUDED_WINDOW_MANAGER_H
#define INCLUDED_WINDOW_MANAGER_H


#include "lib/bitmap.h"
#include "lib/common.h"
#include "lib/input.h"

struct WindowDataPrivate;

typedef struct
{
    WindowDataPrivate   *_private;
    BitmapRGBA          *bmp;
    bool                windowClosed;
    unsigned int	    fps;
} WindowData;


DLL_API WindowData *g_window;

DLL_API bool GetDesktopRes(int *width, int *height);

// Creates a Window (fullscreen is really just a big window) and a bitmap the size of the window
// to use as the back buffer of a double buffer. Also initializes the InputManager to get key and mouse
// input from the window.
DLL_API bool CreateWin(int width, int height, bool windowed, char const *winName);

// Blit back buffer to screen and update FPS counter.
DLL_API void UpdateWin();

DLL_API void ShowMouse();
DLL_API void HideMouse();


#endif

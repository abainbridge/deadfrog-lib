// This module implements code to open a window. It connects the created window
// to the input system (input.h) so that mouse and keyboard events are 
// captured. It also creates a bitmap the same size as the window to use as
// the back buffer of a double buffer system.

// TODO - rename module to df_window.

#pragma once


#include "df_bitmap.h"
#include "df_common.h"
#include "df_input.h"


struct WindowDataPrivate;


typedef struct
{
    WindowDataPrivate   *_private;
    BitmapRGBA          *bmp;
    bool                windowClosed;
    unsigned int	    fps;
    double              advanceTime;
    // TODO - add an isMinimized flag and use it where you see if (g_window->bmp->width < 100)
} WindowData;


enum WindowType
{
    WT_FULLSCREEN = 0,
    WT_WINDOWED = 1
};


DLL_API WindowData *g_window;

DLL_API bool GetDesktopRes(int *width, int *height);

// Creates a Window (fullscreen is really just a big window) and a bitmap the size of the window
// to use as the back buffer of a double buffer. Also initializes the InputManager to get key and mouse
// input from the window.
DLL_API bool CreateWin(int width, int height, WindowType windowed, char const *winName);

// Blit back buffer to screen and update FPS counter.
DLL_API void UpdateWin();

DLL_API void ShowMouse();
DLL_API void HideMouse();

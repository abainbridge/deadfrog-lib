#ifndef INCLUDED_WINDOW_MANAGER_GDI_H
#define INCLUDED_WINDOW_MANAGER_GDI_H


#include "lib/gfx/bitmap.h"
#include "lib/common.h"
#include "lib/input.h"

struct WindowDataPrivate;

typedef struct
{
    WindowDataPrivate   *_private;
    
    BitmapRGBA          *backBuffer;
    int		            width;
    int		            height;

    bool                windowClosed;

    unsigned int	    fps;
} WindowData;


DLL_API extern WindowData *g_window;

// Creates a Window (even fullscreen is really just a big window) and a bitmap the size of the window
// to use as the back buffer of a double buffer. Also initializes the InputManager to get key and mouse
// input from the window.
DLL_API bool            CreateWin(int x, int y, int _width, int _height, bool _windowed, char const *winName);

// Blit back buffer to screen, poll for user input and update FPS counter.
DLL_API void            AdvanceWin();

DLL_API void            ShowMouse();
DLL_API void            HideMouse();


#endif

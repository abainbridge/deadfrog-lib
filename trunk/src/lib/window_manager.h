#ifndef INCLUDED_WINDOW_MANAGER_GDI_H
#define INCLUDED_WINDOW_MANAGER_GDI_H


#include "lib/gfx/bitmap.h"
#include "lib/common.h"
#include "lib/input.h"

struct BitmapRGBA;
struct InputManager;
struct WindowDataPrivate;

struct WindowData
{
    WindowDataPrivate   *_private;
    
    BitmapRGBA          *bmp;
    unsigned int		width;
    unsigned int		height;

    InputManager        *inputManager;
    bool                windowClosed;

    unsigned int	    fps;
};


DLL_API WindowData *    CreateWin(int x, int y, int _width, int _height, bool _windowed, char const *winName);
DLL_API BitmapRGBA *    AdvanceWin(WindowData *w); // Returns the window's bitmap for you to draw on

DLL_API unsigned int    GetWidth(WindowData *w);
DLL_API unsigned int    GetHeight(WindowData *w);

DLL_API void            BlitBitmap(WindowData *w, BitmapRGBA *bmp, int x, int y);

DLL_API void            ShowMouse(WindowData *w);
DLL_API void            HideMouse(WindowData *w);

DLL_API void            SetWindowTitle(WindowData *w, char const *title);


#endif

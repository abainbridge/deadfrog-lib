# deadfrog-lib
A simple graphics, keyboard and mouse library with a C interface.

Deadfrog Lib is a collection of simple portable functions to create a window, get keyboard and mouse input and provide various 2D drawing primitives. It was created due to the frustration I found in just getting a simple interactive program up and running on a PC. The PC should be as easy to program as my ZX Spectrum was in 1982.

It currently supports Windows and Linux/X11.

## Functions include

* Window create/destroy.
* 32-bit RGBA bitmap creation.
* Fast software based drawing routines (pixels, lines, ellipses, polygons, text, blit, masked-blit etc).
* Alpha blending support.
* Anti-aliased polygon drawing.
* Mouse and keyboard input.
* Load and save BMP files.

## Advantages

* Simple API.
* Simple source code.
* Fast (for a simple, portable, software renderer).
* Depends on no third-party libraries.
* It's tiny. Less than 6000 lines of code.
* Reliable results - unlike hardware accelerated drawing where the exact pixel output can change depending on the graphics card or even the driver version.
* Easy to debug. Unlike OpenGl/DirectX etc, you can step through all the code in the debugger to see why your rendering code isn't working.
* Written in C++ with a C interface (to make it easy to use from other languages).
* Permissive BSD licence.
* Easy to port to other platforms.

## Design Details

The library does not make use of any hardware acceleration on a graphics accelerator card. With modern CPU speeds, it is still possible to do 1920x1200 full-screen animation at 60 Hz. For simple operations like pixels and lines etc, the CPU only approach can be faster than a GPU accelerated approach because the overhead of communicating with the graphics card is larger than just doing the work on the CPU.

## Simplifying Assumptions

To keep the code simple, Deadfrog Lib makes the following assumptions:

* Only 32-bit per pixel is supported.
* The display is always double buffered. All rendering occurs to a bitmap in main memory. This can then be blitted to the screen/window very quickly.

## Performance

Below is some performance data of various functions rendering to an in-memory frame buffer. Nothing is visible on the screen. The frame buffer would need to be blitted to the screen for the results to be visible. (Performance of blitting the frame buffer to the screen on my Windows box is around 6 billion pixels per second - eg blitting 1920x1200 to the screen 60 times per second results in about 2% CPU load.)

Running on Windows 10 on a single core of an Intel Core i3 530 @ 2.93 GHz, built with MSVC 2013, targetting x64:

    FUNCTION        UNIT                    Rate        NOTES
    -------------------------------------------------------------------------------------
    Put pixel       million per sec         125
    Put pixel       million per sec         95          Alpha blended
    Line draw       million pixels per sec  650
    Rectangle fill  million pixels per sec  5700
    Text render     million chars per sec   41          7x13 pixel bitmap font
    Swap buffers    number per sec          1260        Bitmap size 1280x1024

## Hello World Example

~~~~c++
#include "df_bitmap.h"
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

int main() { 
    DfWindow *win = CreateWin(800, 600, WT_WINDOWED_RESIZEABLE, "Hello World Example");
    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        BitmapClear(win->bmp, g_colourBlack);
        InputPoll(win);

        unsigned x = 100;
        unsigned y = 100;
        DfColour col = g_colourWhite;
        DrawTextSimple(g_defaultFont, col, win->bmp, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, win->bmp, win->bmp->width - 5, 0, "FPS:%i", win->fps);

        UpdateWin(win);
        WaitVsync();
    }
}
~~~~

## Projects using Deadfrog-lib

* [Code Trowel](http://deadfrog.co.uk) - A programmer's text editor.
* [Sound Shovel](https://github.com/abainbridge/sound_shovel) - A very incomplete WAV file editor.

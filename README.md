# deadfrog-lib
A simple graphics, keyboard and mouse library with a C interface.

Deadfrog Lib is a collection of simple portable functions to create a window, get keyboard and mouse input and provide various 2D drawing primitives. It was created due to the frustration I found in just getting a simple interactive program up and running on a PC. The PC should be as easy to program as my ZX Spectrum was in 1982.

## Functions include

* Window creation.
* 32-bit RGBA bitmap creation.
* Fast software based drawing routines (pixels, lines, ellipses, polygons, text, blit, masked-blit etc).
* Alpha blending support.
* Anti-aliased text and polygon drawing.
* Mouse and keyboard input.
* Load and save BMP files.

## Advantages

* Simple API.
* Simple source code.
* Fast (for a simple, portable, software renderer).
* Depends on no third-party libraries.
* It's tiny. Less than 5000 lines of code. The DLL is about 15K bytes when dynamically linked against the Visual Studio runtime libraries.
* Reliable results - unlike hardware accelerated drawing where the exact pixel output can change depending on the graphics card or even the driver version.
* Easy to debug. Unlike OpenGl/DirectX etc, you can step through all the code in the debugger to see why your rendering code isn't working.
* Written in C++ with a C interface (to make it easy to use from other languages).
* Permissive BSD licence.
* Easy to port to other platforms (but currently limited to Windows).

## Design Details

The library does not make use of any hardware acceleration on a graphics accelerator card. With modern CPU speeds, it is still possible to do 1920x1200 full-screen animation at 60 Hz. For simple operations like pixels and lines etc, the CPU only approach can be faster than a GPU accelerated approach because the overhead of communicating with the graphics card is larger than just doing the work on the CPU.

## Simplifying Assumptions

To keep the code simple, Deadfrog Lib makes the following assumptions:

* Everyone has enough RAM to hold a 32-bit frame buffer.
* An application can only have one window.
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
    Text render     million chars per sec   41          8 point, fixed width font
    Text render     million chars per sec   8           8 point, fixed width font, antialiased
    Swap buffers    number per sec          1260        Bitmap size 1280x1024

## Hello World Example

~~~~c++
int main() 
{ 
    bool windowed = true; 
    DfCreateWin(800, 600, windowed, "Hello World Example"); 
    DfFont *font = DfCreateFont("Courier New", 12);

    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        // Get mouse and keyboard events from OS
        InputManagerAdvance();

        // Clear the back buffer
        BitmapClear(g_window->bmp, g_colourBlack);

        // Print "Hello World!"
        DfColour col = Colour(255, 128, 0);
        unsigned x = 100;
        unsigned y = 100;
        DrawTextSimple(font, col, g_window->bmp, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        // Blit back buffer to screen
        UpdateWin();

        // Wait until the window manager is ready for another frame.
        WaitVsync();
    }

    return 0;
}
~~~~

## Project Information

The project was created on Apr 10, 2012.

Deadfrog Lib is a collection of simple portable functions to create a window, get keyboard and mouse input and provide various 2D drawing primitives. It was created due to the frustration I found in just getting a simple interactive program up and running on a PC. The PC should be as easy to program as my ZX Spectrum was in 1982.

It is a little like SDL, but smaller and with useful features like line drawing and text rendering that are oddly missing from SDL.

### Functions include ###
  * Window creation.
  * 32-bit RGBA bitmap creation.
  * Fast software based drawing routines (pixels, lines, polygons, text, blit, masked-blit etc).
  * Some alpha blending support.
  * Mouse and keyboard input.
  * Load and save BMP files.

### Advantages ###
  * Simple easy to use API.
  * Predictable performance that scales linearly with CPU speed, unlike hardware accelerated drawing whose speed can vary by orders of magnitude due depending on graphics driver version.
  * It's tiny. Less than 3000 lines of code. The DLL is about 8K bytes when dynamically linked against the Visual Studio runtime libraries.
  * Written in C++ with a C interface (to make it easy to use from other languages).
  * Python bindings provided.
  * Permissive BSD licence.
  * Depends on no third-party libraries.
  * Easy to port to other platforms (Currently code is limited to Win32).

### Design Details ###
The library does not make use of any hardware acceleration on a graphics accelerator card. With modern CPU speeds, it is still possible to do 1920x1200 full-screen animation at 60 Hz. For simple operations like pixels and lines etc, the CPU only approach can be faster than a GPU accelerated approach because the overhead of communicating with the graphics card is larger than just doing the work on the CPU.

### Simplifying Assumptions ###
To keep the code simple, Deadfrog Lib makes the following assumptions:

  * Everyone has enough RAM to hold a 32-bit frame buffer.
  * An application can only have one window.
  * The display is always double buffered. All rendering occurs to a bitmap in main memory. This can then be blitted to the screen/window very quickly.

### Performance ###
Below is some performance data of various functions rendering to an in-memory frame buffer. Nothing is visible on the screen. The frame buffer would need to be blitted to the screen for the results to be visible. (Performance of blitting the frame buffer to the screen on my Windows box is around 6 billion pixels per second - eg blitting 1920x1200 to the screen 60 times per second results in about 2% CPU load.)

Figures in the GDI column result from using a device independent bitmap.

Running on Windows 7 on a single core of an Intel Core i3 530 @ 2.93 GHz:

| FUNCTION       | UNIT                   | Deadfrog | GDI  | NOTES |
|:---------------|:-----------------------|:---------|:-----|:------|
| Put pixel      | million per sec        | 50       | 1    |       |
| Line draw      | million pixels per sec | 600      | 150  |       |
| Rectangle fill | million pixels per sec | 2700     | 4300 |       |
| Text render    | million chars per sec  | 14       | 4.5  | 8 point, fixed width font |

Running on Windows XP on a single core of an Intel i5-2430M @ 3.0 GHz:
| FUNCTION       | UNIT                   | Deadfrog | GDI  | NOTES |
|:---------------|:-----------------------|:---------|:-----|:------|
| Put pixel      | million per sec        | 1290     | 2    |       |
| Line draw      | million pixels per sec | 600      | 210  |       |
| Rectangle fill | million pixels per sec | 3100     | 2470 |       |
| Text render    | million chars per sec  | 17       | 8    | 8 point, fixed width font |

### Missing Features ###
  * Anti-aliasing

### Hello World Example ###
```
int main()
{
    bool windowed = true;
    CreateWin(800, 600, windowed, "Hello World Example");
    TextRenderer *font = CreateTextRenderer("Courier New", 12);

    while (!g_window->windowClosed && !g_inputManager.keys[KEY_ESC])
    {
        // Get mouse and keyboard events from OS
        InputManagerAdvance();

        // Clear the back buffer
        ClearBitmap(g_window->bmp, g_colourBlack);

        // Print "Hello World!"
        RGBAColour col = Colour(255, 128, 0);
        unsigned x = 100;
        unsigned y = 100;
        DrawTextSimple(font, col, g_window->bmp, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        // Blit back buffer to screen
        UpdateWin();

        SleepMillisec(10);
    }

    return 0;
}
```
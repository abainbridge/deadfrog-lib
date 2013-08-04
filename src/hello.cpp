#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/gfx/bitmap.h"
#include "lib/gfx/text_renderer.h"
#include "lib/window_manager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	CreateWin(100, 10, 800, 600, true, "Hello World Example");
    TextRenderer *font = CreateTextRenderer("Courier New", 12);

    int x = 100;
    int y = 100;
    while (!g_window->windowClosed && !g_inputManager.keys[KEY_ESC])
    {
        AdvanceWin(); // Read user input, blit back buffer to screen, etc
        ClearBitmap(g_window->backBuffer, g_colourBlack);

        x += 1;
        y += 2;
        if (x > g_window->width)
            x = 0;
        if (y > g_window->height)
            y = 0;

        RGBAColour col = Colour(x & 255, y & 255, (x*y) & 255);
        DrawTextSimple(font, col, g_window->backBuffer, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, g_window->backBuffer, g_window->width - 5, 0, "FPS:%i", g_window->fps);

        SleepMillisec(10);
    }

    return 0;
}

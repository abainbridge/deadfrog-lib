#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/bitmap.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	CreateWin(100, 10, 800, 600, true, "Hello World Example");
    TextRenderer *font = CreateTextRenderer("Courier New", 12);

    unsigned x = 100;
    unsigned y = 100;
    while (!g_window->windowClosed && !g_inputManager.keys[KEY_ESC])
    {
        ClearBitmap(g_window->bmp, g_colourBlack);
        InputManagerAdvance();

        x += 1;
        y += 2;
        if (x > g_window->bmp->width)
            x = 0;
        if (y > g_window->bmp->height)
            y = 0;

        RGBAColour col = Colour(x & 255, y & 255, (x*y) & 255);
        DrawTextSimple(font, col, g_window->bmp, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        UpdateWin();
        SleepMillisec(10);
    }

    return 0;
}

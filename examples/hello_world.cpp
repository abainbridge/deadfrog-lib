#include "df_bitmap.h"
#include "df_font.h"
#include "df_time.h"
#include "df_window.h"
#include "fonts/df_mono.h"


void HelloWorldMain()
{
	CreateWin(800, 600, WT_WINDOWED, "Hello World Example");
    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    unsigned x = 100;
    unsigned y = 100;
    while (!g_window->windowClosed && !g_window->input.keys[KEY_ESC])
    {
        BitmapClear(g_window->bmp, g_colourBlack);
        InputPoll();
		
        x += 1;
        y += 2;
        if (x > g_window->bmp->width)
            x = 0;
        if (y > g_window->bmp->height)
            y = 0;

        DfColour col = Colour(x & 255, y & 255, (x*y) & 255);
        DrawTextSimple(g_defaultFont, col, g_window->bmp, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        UpdateWin();
        WaitVsync();
    }
}

#include "df_bitmap.h"
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"


void HelloWorldMain()
{
    DfWindow *win = CreateWin(800, 600, WT_WINDOWED_RESIZEABLE, "Hello World Example");

    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    unsigned x = 100;
    unsigned y = 100;
    while (!win->windowClosed && !win->input.keys[KEY_ESC])
    {
        BitmapClear(win->bmp, g_colourBlack);
        InputPoll(win);

        x += 1;
        y += 2;
        if (x > win->bmp->width)
            x = 0;
        if (y > win->bmp->height)
            y = 0;

        DfColour col = Colour(x & 255, y & 255, (x*y) & 255);
        DrawTextSimple(g_defaultFont, col, win->bmp, x, y, "Hello World!");

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, win->bmp, win->bmp->width - 5, 0, "FPS:%i", win->fps);

        UpdateWin(win);
        WaitVsync();
    }
}

#include "df_bitmap.h"
#include "df_window.h"


int main()
{
    CreateWin(800, 600, WT_WINDOWED, "Clipping test");

    BitmapClear(win->bmp, g_colourBlack);

    // Set clip rect as 100,150 -> 700,450
    RectOutline(win->bmp, 100, 150, 600, 300, Colour(255, 50, 50));
    SetClipRect(win->bmp, 100, 150, 600, 300);

    //
    // Y first

    // Entirely above and below. Shouldn't draw any pixels.
    RectFill(win->bmp, 150, -10, 50, 160, g_colourWhite);
    RectFill(win->bmp, 150, 450, 50, 160, g_colourWhite);

    // Overlapping top. Should be clipped.
    RectFill(win->bmp, 250, 10, 50, 200, g_colourWhite);

    // Overlapping bottom. Should be clipped.
    RectFill(win->bmp, 250, 390, 50, 200, g_colourWhite);

    // Overlapping top and bottom. Should be clipped.
    RectFill(win->bmp, 350, 10, 50, 500, g_colourWhite);

    // Not clipped
    RectFill(win->bmp, 450, 200, 50, 200, g_colourWhite);

    while (1)
    {
        InputPoll(win);
        UpdateWin(win);
        WaitVsync();
        if (win->windowClosed) return;
        if (win->input.numKeyDowns > 0) break;
    }


    // 
    // Now X

    BitmapClear(win->bmp, g_colourBlack);
    RectOutline(win->bmp, 100, 150, 600, 300, Colour(255, 50, 50));

    // Entirely to left and entirely right.
    RectFill(win->bmp, 10, 180, 80, 50, g_colourWhite);
    RectFill(win->bmp, 710, 180, 130, 50, g_colourWhite);

    // Overlapping left.
    RectFill(win->bmp, 10, 270, 130, 50, g_colourWhite);

    // Overlapping right.
    RectFill(win->bmp, 660, 270, 130, 50, g_colourWhite);

    // Overlapping left and right.
    RectFill(win->bmp, 10, 360, 750, 50, g_colourWhite);

    // Not clipped.
    RectFill(win->bmp, 350, 180, 100, 50, g_colourWhite);

    while (1)
    {
        InputPoll(win);
        UpdateWin(win);
        WaitVsync();
        if (win->windowClosed) return;
        if (win->input.numKeyDowns > 0) break;
    }
}

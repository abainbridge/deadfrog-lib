#include "df_bitmap.h"
#include "df_window.h"


int main()
{
    CreateWin(800, 600, WT_WINDOWED, "Clipping test");

    BitmapClear(g_window->bmp, g_colourBlack);

    // Set clip rect as 100,150 -> 700,450
    RectOutline(g_window->bmp, 100, 150, 600, 300, Colour(255, 50, 50));
    SetClipRect(g_window->bmp, 100, 150, 600, 300);

    //
    // Y first

    // Entirely above and below. Shouldn't draw any pixels.
    RectFill(g_window->bmp, 150, -10, 50, 160, g_colourWhite);
    RectFill(g_window->bmp, 150, 450, 50, 160, g_colourWhite);

    // Overlapping top. Should be clipped.
    RectFill(g_window->bmp, 250, 10, 50, 200, g_colourWhite);

    // Overlapping bottom. Should be clipped.
    RectFill(g_window->bmp, 250, 390, 50, 200, g_colourWhite);

    // Overlapping top and bottom. Should be clipped.
    RectFill(g_window->bmp, 350, 10, 50, 500, g_colourWhite);

    // Not clipped
    RectFill(g_window->bmp, 450, 200, 50, 200, g_colourWhite);

    while (1)
    {
        InputPoll();
        UpdateWin();
        WaitVsync();
        if (g_window->windowClosed) return;
        if (g_window->input.numKeyDowns > 0) break;
    }


    // 
    // Now X

    BitmapClear(g_window->bmp, g_colourBlack);
    RectOutline(g_window->bmp, 100, 150, 600, 300, Colour(255, 50, 50));

    // Entirely to left and entirely right.
    RectFill(g_window->bmp, 10, 180, 80, 50, g_colourWhite);
    RectFill(g_window->bmp, 710, 180, 130, 50, g_colourWhite);

    // Overlapping left.
    RectFill(g_window->bmp, 10, 270, 130, 50, g_colourWhite);

    // Overlapping right.
    RectFill(g_window->bmp, 660, 270, 130, 50, g_colourWhite);

    // Overlapping left and right.
    RectFill(g_window->bmp, 10, 360, 750, 50, g_colourWhite);

    // Not clipped.
    RectFill(g_window->bmp, 350, 180, 100, 50, g_colourWhite);

    while (1)
    {
        InputPoll();
        UpdateWin();
        WaitVsync();
        if (g_window->windowClosed) return;
        if (g_window->input.numKeyDowns > 0) break;
    }
}

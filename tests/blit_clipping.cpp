#include "df_bitmap.h"
#include "df_bmp.h"
#include "df_window.h"


void HelloWorldMain()
{
    CreateWin(800, 600, WT_WINDOWED, "Clipping test");

    BitmapClear(g_window->bmp, g_colourBlack);

    DfBitmap *dog = LoadBmp("../../marlieses_dog.bmp");
    if (!dog)
        dog = LoadBmp("marlieses_dog.bmp");
    ReleaseAssert(dog, "Couldn't load marlieses_dog.bmp");

    // Set clip rect as 100,150 -> 700,450
    RectOutline(g_window->bmp, 100, 160, 600, 280, Colour(55, 55, 255));
    SetClipRect(g_window->bmp, 100, 160, 600, 280);

    //
    // Y first

    // Entirely above and below. Shouldn't draw any pixels.
    QuickBlit(g_window->bmp, 50, -310, dog);
    QuickBlit(g_window->bmp, 50, 450, dog);

    // Overlapping top. Should be clipped.
    QuickBlit(g_window->bmp, -100, -60, dog);

    // Overlapping bottom. Should be clipped.
    QuickBlit(g_window->bmp, -100, 390, dog);

    // Overlapping top and bottom. Should be clipped.
    QuickBlit(g_window->bmp, 250, 150, dog);

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
    SetClipRect(g_window->bmp, 260, 100, 280, 380);
    RectOutline(g_window->bmp, 260, 100, 280, 380, Colour(55, 55, 255));

    // Entirely to left and entirely right.
    QuickBlit(g_window->bmp, -100, 160, dog);
    QuickBlit(g_window->bmp, 610, 140, dog);

    // Overlapping left.
    QuickBlit(g_window->bmp, 0, 150, dog);

    // Overlapping right.
    QuickBlit(g_window->bmp, 480, 150, dog);

    // Overlapping left and right.
    QuickBlit(g_window->bmp, 250, 130, dog);
 
    while (1)
    {
        InputPoll();
        UpdateWin();
        WaitVsync();
        if (g_window->windowClosed) return;
        if (g_window->input.numKeyDowns > 0) break;
    }
}

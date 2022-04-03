#include "df_bitmap.h"
#include "df_bmp.h"
#include "df_window.h"


void HelloWorldMain()
{
    CreateWin(800, 600, WD_WINDOWED_RESIZEABLE, "Clipping test");

    BitmapClear(win->bmp, g_colourBlack);

    DfBitmap *dog = LoadBmp("../../marlieses_dog.bmp");
    if (!dog)
        dog = LoadBmp("marlieses_dog.bmp");
    ReleaseAssert(dog, "Couldn't load marlieses_dog.bmp");

    // Set clip rect as 100,150 -> 700,450
    RectOutline(win->bmp, 100, 160, 600, 280, Colour(55, 55, 255));
    SetClipRect(win->bmp, 100, 160, 600, 280);

    //
    // Y first

    // Entirely above and below. Shouldn't draw any pixels.
    QuickBlit(win->bmp, 50, -310, dog);
    QuickBlit(win->bmp, 50, 450, dog);

    // Overlapping top. Should be clipped.
    QuickBlit(win->bmp, -100, -60, dog);

    // Overlapping bottom. Should be clipped.
    QuickBlit(win->bmp, -100, 390, dog);

    // Overlapping top and bottom. Should be clipped.
    QuickBlit(win->bmp, 250, 150, dog);

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
    SetClipRect(win->bmp, 260, 100, 280, 380);
    RectOutline(win->bmp, 260, 100, 280, 380, Colour(55, 55, 255));

    // Entirely to left and entirely right.
    QuickBlit(win->bmp, -100, 160, dog);
    QuickBlit(win->bmp, 610, 140, dog);

    // Overlapping left.
    QuickBlit(win->bmp, 0, 150, dog);

    // Overlapping right.
    QuickBlit(win->bmp, 480, 150, dog);

    // Overlapping left and right.
    QuickBlit(win->bmp, 250, 130, dog);
 
    while (1)
    {
        InputPoll(win);
        UpdateWin(win);
        WaitVsync();
        if (win->windowClosed) return;
        if (win->input.numKeyDowns > 0) break;
    }
}

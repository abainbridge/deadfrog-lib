#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include "df_message_dialog.h"

void TwoWindowsMain()
{
    MessageDialog("About Code Trowel", 
        "     Beta Version\n"
        "     Feb 18 2022\n"
        "\n"
        "http://deadfrog.co.uk", MsgDlgTypeOk);

    DfWindow *win = CreateWin(600, 400, WT_WINDOWED_RESIZEABLE, "Window 1");
    DfWindow *win2 = CreateWin(600, 400, WT_WINDOWED_RESIZEABLE, "Window the other");

    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    unsigned x = 100;
    unsigned y = 100;
    while (!win->windowClosed && !win->input.keys[KEY_ESC] &&
           !win2->windowClosed && !win2->input.keys[KEY_ESC])
    {
        InputPoll(win);
        InputPoll(win2);
		
        BitmapClear(win->bmp, g_colourBlack);
        BitmapClear(win2->bmp, g_colourBlack);

        x += 1;
        y += 2;
        if (x > win->bmp->width)
            x = 0;
        if (y > win->bmp->height)
            y = 0;

        DfColour col = Colour(x & 255, y & 255, (x*y) & 255);
        DrawTextSimple(g_defaultFont, col, win->bmp, x, y, "Window 1");

        DfColour col2 = Colour(y & 255, x & 255, (x*y) & 255);
        DrawTextSimple(g_defaultFont, col2, win2->bmp, x, y, "Window the other");

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, win->bmp, win->bmp->width - 5, 0, "FPS:%i", win->fps);

        UpdateWin(win);
        UpdateWin(win2);

        WaitVsync();
    }
}

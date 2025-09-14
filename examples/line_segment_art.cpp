#include "df_bitmap.h"
#include "df_window.h"

#include <math.h>


void LineSegmentArtMain() {
    DfWindow *win = CreateWin(960, 1247, WT_WINDOWED_FIXED, "Line Segment Art");
    BitmapClear(win->bmp, g_colourWhite);
    
    for (double y = 110; y < win->bmp->height - 80; y += 30) {
        double xInc = 28;
        double angle = 0.3;
        double lineLen = 28;
        bool gettingCloser = true;
        for (double x = sin(y * 0.015) * 120; x < win->bmp->width - 70;) {
            double dx = cos(angle) * lineLen;
            double dy = sin(angle) * lineLen;
            DrawLine(win->bmp, x, y, x + dx, y + dy, g_colourBlack);

            if (gettingCloser) {
                xInc *= 0.95;
                angle += 0.02;
                if (xInc < 0.6) {
                    gettingCloser = false;
                    xInc = 2.0;
                }
            }
            else {
                xInc *= 1.05;
                angle += 0.13;
            }
            x += xInc;
        }
    }

    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        InputPoll(win);
        UpdateWin(win);
        WaitVsync();
    }
}

// Deadfrog lib headers
#include "df_window.h"

// Standard headers
#include <stdlib.h>


void FractalFernMain()
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
//    CreateWin(width - 200, height - 100, WT_WINDOWED, "Fractal Fern");
    CreateWin(width, height, WT_FULLSCREEN, "Fractal Fern");
    HideMouse();

    int antialiasFactor = 3;
    unsigned bw = g_window->bmp->width * antialiasFactor;
    DfBitmap *bmp = DfCreateBitmap(bw, g_window->bmp->height * antialiasFactor);
    ClearBitmap(bmp, g_colourBlack);
    double scale = bmp->width * 0.09;
    double x = 0;
    double y = 0;

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_input.keyDowns[KEY_ESC])
    {
        InputManagerAdvance();

        for (int i = 0; i < 400000; i++) {
            int rnd = rand() % 100;

            if (rnd < 1) {
                x = 0;
                y *= 0.16;
            }
            else if (rnd < 86) {
                double t = 0.85 * x + 0.04 * y;
                y = -0.04 * x + 0.85 * y + 1.6;
                x = t;
            }
            else if (rnd < 93) {
                double t = 0.2 * x - 0.26 * y;
                y = 0.23 * x + 0.22 * y + 1.6;
                x = t;
            }
            else {
                double t = -0.15 * x + 0.28 * y;
                y = 0.26 * x + 0.24 * y + 0.44;
                x = t;
            }

            PutPixUnclipped(bmp, scale * (y + 0.56), scale * (x + 3.3), Colour(60, 255, 20, 10));
        }

        ScaleDownBlit(g_window->bmp, 0, 0, antialiasFactor, bmp);
        UpdateWin();
    }
}

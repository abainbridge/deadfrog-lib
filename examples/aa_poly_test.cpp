// Deadfrog lib headers
#include "df_font.h"
#include "fonts/df_mono.h"
#include "df_polygon_aa.h"
#include "df_time.h"
#include "df_window.h"

// Standard headers
#include <math.h>


inline int Round(float a)
{
    return floorf(a + 0.5f);
}


void Chevron(DfBitmap *bmp, int xOffset, int yOffset, float zoom) {
    // The left arrow is drawn as two convex polygons with anti-clockwise winding order.
    //
    //        x0      x1   x2       x3
    //
    //
    //                    3         2
    // y0                  * * * * *
    //                   *       *
    //                 *       *
    //               *       *
    //             *       *
    //           *     1 *
    // y1    0 * * * * *
    //           *     3 *
    //             *       *
    //               *       *
    //                 *       *
    //                   *       *
    // y2                  * * * * *
    //                    1         2

    int left = 10 * 16 + xOffset;
    int top = 10 * 16 + yOffset;
    int width = 15;
    int x0 = left + Round(width * 5.1f * zoom);
    int x1 = left + Round(width * 7.1f * zoom);
    int x2 = left + Round(width * 8.3f * zoom);
    int x3 = left + Round(width * 10.5f * zoom);
    int y0 = top + Round(width * 4.2f * zoom);
    int y1 = top + Round(width * 8.0f * zoom);
    int y2 = top + Round(width * 11.5f * zoom);

    {
        DfVertex verts[4] = {
            { x0, y1 },
            { x1, y1 },
            { x3, y0 },
            { x2, y0 }
        };
        FillPolygonAa(bmp, verts, 4, g_colourBlack);
    }

    {
        DfVertex verts[5] = {
            { x0, y1 },
            { x2, y2 },
            { x2, y2 },
            { x3, y2 },
            { x1, y1 }
        };
        FillPolygonAa(bmp, verts, 5, g_colourBlack);
    }
}


void ManyChevrons(DfBitmap *bmp) {
    for (float zoom = 0.5f; zoom < 5.0f; zoom += 0.5f) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                Chevron(bmp, x, y, zoom);
            }
        }
    }
}


void AaPolyTestMain()
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
    CreateWin(1280, 1024, WT_WINDOWED, "AA Poly Test");

    g_defaultFont = LoadFontFromMemory(deadfrog_mono_7x13, sizeof(deadfrog_mono_7x13));

    DfBitmap *bmp = BitmapCreate(100, 100);

    // Continue to display the window until the user presses escape or clicks the close icon
    int xOffset = 0;
    int yOffset = 200;
    double zoom = 1.0;
    double avgDuration = 0.0;
    DfColour bgCol1 = Colour(200, 200, 200);
    DfColour bgCol2 = Colour(180, 180, 180);
    while (!g_window->windowClosed && !g_input.keyDowns[KEY_ESC])
    {
        BitmapClear(g_window->bmp, bgCol1);
        BitmapClear(bmp, bgCol2);
        double start = GetRealTime();
#if 0
        ManyChevrons(bmp);  // Use this version for benchmarking.
#else
        Chevron(bmp, xOffset, yOffset, zoom); // Use this version for interactive testing.
#endif
        double thisDuration = (GetRealTime() - start) * 1000.0;
        QuickBlit(g_window->bmp, 0, 0, bmp);
        ScaleUpBlit(g_window->bmp, 100, 0, 6, bmp);

        xOffset -= g_input.keyDowns[KEY_LEFT];
        xOffset += g_input.keyDowns[KEY_RIGHT];
        yOffset -= g_input.keyDowns[KEY_UP];
        yOffset += g_input.keyDowns[KEY_DOWN];
        zoom += 0.01 * g_input.keyDowns[KEY_EQUALS];
        zoom -= 0.01 * g_input.keyDowns[KEY_MINUS];

        avgDuration = 0.99 * avgDuration + 0.01 * thisDuration;
        DrawTextLeft(g_defaultFont, g_colourBlack, g_window->bmp, 10, 110, "Duration = %.2f ms", avgDuration);

        InputPoll();
        UpdateWin();
//        WaitVsync();
    }
}

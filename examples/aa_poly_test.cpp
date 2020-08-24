// Deadfrog lib headers
#include "df_polygon_aa.h"
#include "df_window.h"

// Standard headers
#include <math.h>


inline int Round(float a)
{
    return floorf(a + 0.5f);
}


void Chevron(DfBitmap *bmp) {
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

    int left = 10;
    int top = 10;
    int m_buttonWidth = 15;
    int x0 = left * 16 + Round(m_buttonWidth * 5.1f);
    int x1 = left * 16 + Round(m_buttonWidth * 7.1f);
    int x2 = left * 16 + Round(m_buttonWidth * 8.3f);
    int x3 = left * 16 + Round(m_buttonWidth * 10.5f);
    int y0 = top * 16 + Round(m_buttonWidth * 4.2f);
    int y1 = top * 16 + Round(m_buttonWidth * 8.0f);
    int y2 = top * 16 + Round(m_buttonWidth * 11.5f);

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
        DfVertex verts[4] = {
            { x0, y1 },
            { x2, y2 },
            { x3, y2 + 1 },
            { x1, y1 }
        };
        FillPolygonAa(bmp, verts, 4, g_colourBlack);
    }
}


void AaPolyTestMain()
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
    CreateWin(1280, 1024, WT_WINDOWED, "AA Poly Test");

    DfBitmap *bmp = BitmapCreate(100, 100);
    BitmapClear(bmp, Colour(180, 180, 180));

    Chevron(bmp);
    QuickBlit(g_window->bmp, 0, 0, bmp);
    ScaleUpBlit(g_window->bmp, 100, 0, 6, bmp);

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_input.keyDowns[KEY_ESC])
    {
        InputPoll();
        UpdateWin();
        WaitVsync();
    }
}

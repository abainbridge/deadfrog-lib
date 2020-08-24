#include "bitmap.h"

void DrawLine2(BitmapRGBA *bmp, int x1, int y1, int x2, int y2, RGBAColour colour)
{
//     The code below is adapted from a Bresenham implementation by David
//     Brackeen (http://www.brackeen.com/vga/shapes.html).
    // It is about half as fast as the one in df_bitmap.cpp

    int dx = x2 - x1;
    int dy = y2 - y1;
    int dxAbs = abs(dx);
    int dyAbs = abs(dy);
    int signDx = (dx > 0 ? 1 : -1);
    int signDy = (dy > 0 ? 1 : -1);
    int x = dyAbs >> 1;
    int y = dxAbs >> 1;
    int px = x1;
    int py = y1;

    PutPixUnclipped(bmp, px, py, colour);

    if (dxAbs >= dyAbs) // the line is more horizontal than vertical
    {
        for(int i = 0; i < dxAbs; i++)
        {
            y += dyAbs;
            if (y >= dxAbs)
            {
                y -= dxAbs;
                py += signDy;
            }
            px += signDx;
            PutPixUnclipped(bmp, px, py, colour);
        }
    }
    else // the line is more vertical than horizontal
    {
        for(int i = 0; i < dyAbs; i++)
        {
            x += dxAbs;
            if (x >= dyAbs)
            {
                x -= dyAbs;
                px += signDx;
            }
            py += signDy;
            PutPixUnclipped(bmp, px, py, colour);
        }
    }
}

#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/bitmap.h"
#include "lib/window_manager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    CreateWin(100, 10, 1920, 1200, false, "Broken Mandelbrot Example");
    HideMouse();

    double zoomFactor = 4.0 / (double)g_window->height;

    for (int y = 0; y < g_window->height; y++)
    {
        double ci = (double)y * zoomFactor - 2.0;
        for (int x = 0; x < g_window->width; x++)
        {
            double zr = 0.0;
            double zi = 0.0;
            double cr = (double)x * zoomFactor - 3.5;
            int i;
            for (i = 0; i < 256; i++)
            {
                double newZr = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = newZr;

                if (zr + zi >= 63)
                    break;
            }
            i *= 4;
            PutPix(g_window->backBuffer, x, y, Colour(i,i,i));
        }

        if ((y & 7) == 7)
        {
            if (g_window->windowClosed || g_inputManager.keys[KEY_ESC])
                return 0;
            AdvanceWin();
        }
    }

    while (!g_window->windowClosed && !g_inputManager.keys[KEY_ESC])
    {
        AdvanceWin();
        SleepMillisec(100);
    }

    return 0;
}

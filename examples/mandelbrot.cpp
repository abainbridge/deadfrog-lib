#include "df_time.h"
#include "df_bitmap.h"
#include "df_window.h"
#include <windows.h>


void MandelbrotMain()
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
    CreateWin(width - 200, height - 90, WT_WINDOWED, "Broken Mandelbrot Example");
    HideMouse();

    double zoomFactor = 4.0 / (double)g_window->bmp->height;

    // Draw the broken Mandelbrot Set, one line at a time
    for (unsigned y = 0; y < g_window->bmp->height; y++)
    {
        double ci = (double)y * zoomFactor - 2.0;
        
        // For each pixel on this line...
        for (unsigned x = 0; x < g_window->bmp->width; x++)
        {
            double zr = 0.0;
            double zi = 0.0;
            double cr = (double)x * zoomFactor - 3.5;
            int i;
            for (i = 0; i < 64; i++)
            {
                double newZr = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = newZr;

                if (zr + zi >= 63)
                    break;
            }
            i *= 4;
            PutPix(g_window->bmp, x, y, Colour(i,i,i));
        }

        // Every 8 lines, copy the bitmap to the screen
        if ((y & 7) == 7)
        {
            // Abort drawing the set if the user presses escape or clicks the close icon
            if (g_window->windowClosed || g_input.keys[KEY_ESC])
                return;
            UpdateWin();
        }
    }

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        InputPoll();
        SleepMillisec(100);
    }
}

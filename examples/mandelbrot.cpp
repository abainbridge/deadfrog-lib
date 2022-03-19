#include "df_time.h"
#include "df_bitmap.h"
#include "df_window.h"


void MandelbrotMain()
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
    DfWindow *win = CreateWin(width - 200, height - 90, WT_WINDOWED, "Broken Mandelbrot Example");
    HideMouse(win);

    double zoomFactor = 4.0 / (double)win->bmp->height;

    // Draw the broken Mandelbrot Set, one line at a time
    for (unsigned y = 0; y < win->bmp->height; y++)
    {
        double ci = (double)y * zoomFactor - 2.0;
        
        // For each pixel on this line...
        for (unsigned x = 0; x < win->bmp->width; x++)
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
            PutPix(win->bmp, x, y, Colour(i,i,i));
        }

        // Every 8 lines, copy the bitmap to the screen
        if ((y & 7) == 7)
        {
            // Abort drawing the set if the user presses escape or clicks the close icon
            if (win->windowClosed || win->input.keys[KEY_ESC])
                return;
            UpdateWin(win);
        }
    }

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!win->windowClosed && !win->input.keys[KEY_ESC])
    {
        InputPoll(win);
        SleepMillisec(100);
    }
}

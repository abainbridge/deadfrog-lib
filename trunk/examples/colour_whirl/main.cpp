#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include <limits.h>
#include <time.h>
#include <windows.h>


unsigned available[10000];
unsigned num_available = 0;


inline unsigned Coord(unsigned x, unsigned y) 
{ 
    return (x<<16) | y; 
}


inline unsigned ColourDiff(RGBAColour a, RGBAColour b)
{
    int r = (a.r - b.r)/2;
    r *= r;
    int g = (a.g - b.g);
    g *= g;
    int blue = (a.b - b.b)/2;
    blue *= blue;
    return r + g + blue;
}


// Calculate diff between specified colour and neighbours of specified location
unsigned CalcDiff(unsigned x, unsigned y, RGBAColour target_colour)
{
    unsigned min_diff = UINT_MAX;

    RGBAColour *row = g_window->bmp->pixels + (y-1) * g_window->bmp->width + x;

    unsigned diff = ColourDiff(row[0], target_colour);
    if (diff < min_diff)
        min_diff = diff;

    row += g_window->bmp->width;
    
    diff = ColourDiff(row[-1], target_colour);
    if (diff < min_diff)
        min_diff = diff;

    diff = ColourDiff(row[1], target_colour);
    if (diff < min_diff)
        min_diff = diff;

    row += g_window->bmp->width;

    diff = ColourDiff(row[0], target_colour);
    if (diff < min_diff)
        min_diff = diff;

    return min_diff;
}


unsigned FindBest(RGBAColour c)
{
    unsigned best = 0;

    unsigned min_diff = UINT_MAX;
    for (unsigned i = 0; i < num_available; i++)
    {
        unsigned x = available[i] >> 16;
        unsigned y = available[i] & 0xffff;
        unsigned diff = CalcDiff(x, y, c);
        if (diff < min_diff)
        {
            min_diff = diff;
            best = i;
        }
    }

    return best;
}


RGBAColour alreadyAddedCol = g_colourBlack;


inline void AddIfNew(unsigned x, unsigned y)
{
    if (GetPix(g_window->bmp, x, y).c == g_colourBlack.c)
    {
        available[num_available++] = Coord(x, y);
        PutPix(g_window->bmp, x, y, alreadyAddedCol);
    }
}


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
    CreateWin(0, 0, width, height, false, "Colour Whirl Example");
//    CreateWin(100, 30, width - 200, height - 90, true, "Colour Whirl Example");
    HideMouse();
    ClearBitmap(g_window->bmp, g_colourBlack);

    // Create an array of colours
    int const max_component = 64;
    int const scale = 256 / max_component;
    RGBAColour *colours = new RGBAColour [max_component * max_component * max_component];
    int n = 0;
    {
        for (int r = 0; r < max_component; r++)
            for (int g = 0; g < max_component; g++)
                for (int b = 0; b < max_component; b++)
                    colours[n++] = Colour(r * scale, g * scale, b * scale);
    }

    // Shuffle the array
    srand(clock());
    for (int i = 0; i < n - 1; i++) 
    {
        int j = i + rand() / (RAND_MAX / (n - i) + 1);
        RGBAColour t = colours[j];
        colours[j] = colours[i];
        colours[i] = t;
    }

    double start_time = GetHighResTime();

    alreadyAddedCol.a = 1;
    available[num_available++] = Coord(g_window->bmp->width/2, g_window->bmp->height/2);

    for (; n; n--)
    {         
        unsigned i = FindBest(colours[n]);
        unsigned x = available[i] >> 16;
        unsigned y = available[i] & 0xffff;
        if (num_available > 1)
            available[i] = available[num_available - 1];
        num_available--;

        PutPix(g_window->bmp, x, y, colours[n]);

        AddIfNew(x-1, y-1);
        AddIfNew(x  , y-1);
        AddIfNew(x+1, y-1);
        AddIfNew(x-1, y  );
        AddIfNew(x+1, y  );
        AddIfNew(x-1, y+1);
        AddIfNew(x  , y+1);
        AddIfNew(x+1, y+1);

        // Every so often, copy the bitmap to the screen
        if ((n & 511) == 0)
        {
            // Abort drawing the set if the user presses escape or clicks the close icon
            InputManagerAdvance();
            if (g_window->windowClosed || g_inputManager.keyDowns[KEY_ESC])
                return 0;
            UpdateWin();
        }
    }

    double endTime = GetHighResTime();
    TextRenderer *font = CreateTextRenderer("Courier New", 10);
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, 10, "Time taken: %.2f sec. Press ESC.", endTime - start_time);

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_inputManager.keys[KEY_ESC])
    {
        InputManagerAdvance();
        SleepMillisec(100);
    }

    return 0;
}

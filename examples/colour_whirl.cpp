#include "df_time.h"
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"
#include <limits.h>
#include <stdlib.h>


unsigned available[10000];
unsigned num_available = 0;


inline unsigned Coord(unsigned x, unsigned y) 
{ 
    return (x<<16) | y; 
}


inline unsigned ColourDiff(DfColour a, DfColour b)
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
unsigned CalcDiff(unsigned x, unsigned y, DfColour target_colour)
{
    unsigned min_diff = UINT_MAX;

    DfColour *row = g_window->bmp->pixels + (y-1) * g_window->bmp->width + x;

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


unsigned FindBest(DfColour c)
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


DfColour alreadyAddedCol = g_colourBlack;


inline void AddIfNew(unsigned x, unsigned y)
{
    if (GetPix(g_window->bmp, x, y).c == g_colourBlack.c)
    {
        if (x > 1 && x < (g_window->bmp->width-1) &&
            y > 1 && y < (g_window->bmp->height-1))
        {
            if (num_available < 10000)
                available[num_available++] = Coord(x, y);
            PutPix(g_window->bmp, x, y, alreadyAddedCol);
        }
    }
}


void PlaceColour(unsigned x, unsigned y, DfColour colour)
{
    PutPix(g_window->bmp, x, y, colour);

    AddIfNew(x-1, y-1);
    AddIfNew(x  , y-1);
    AddIfNew(x+1, y-1);
    AddIfNew(x-1, y  );
    AddIfNew(x+1, y  );
    AddIfNew(x-1, y+1);
    AddIfNew(x  , y+1);
    AddIfNew(x+1, y+1);
}


void ColourWhirlMain()
{
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
//    CreateWin(width, height, false, "Colour Whirl Example");
    CreateWin(1200, 900, WT_WINDOWED, "Colour Whirl Example");
    BitmapClear(g_window->bmp, g_colourBlack);

    // Create an array of colours
    int const max_component = 100;
    double const scale = 256.0 / max_component;
    DfColour *colours = new DfColour [max_component * max_component * max_component];
    int n = 0;
    {
        for (int r = 0; r < max_component; r++)
            for (int g = 0; g < max_component; g++)
                for (int b = 0; b < max_component; b++)
                    colours[n++] = Colour((int)(r * scale), (int)(g * scale), (int)(b * scale));
    }

    // Shuffle the array
    srand((unsigned)(GetRealTime() * 1000.0));
    for (int i = 0; i < n - 1; i++) 
    {
        int j = i + rand() / (RAND_MAX / (n - i) + 1);
        DfColour t = colours[j];
        colours[j] = colours[i];
        colours[i] = t;
    }

    double start_time = GetRealTime();

    alreadyAddedCol.b = 1;
    PlaceColour(100, 100, colours[n++]);
    PlaceColour(101, 100, colours[n++]);
    PlaceColour(100, 110, colours[n++]);
    PlaceColour(101, 110, colours[n++]);
    PlaceColour(g_window->bmp->width/2, g_window->bmp->height/2, colours[n++]);
 
    for (; n && num_available; n--)
    {         
        unsigned i = FindBest(colours[n]);
        unsigned x = available[i] >> 16;
        unsigned y = available[i] & 0xffff;
        if (num_available > 1)
            available[i] = available[num_available - 1];
        num_available--;

        PlaceColour(x, y, colours[n]);

        // Every so often, copy the bitmap to the screen
        if ((n & 511) == 0)
        {
            // Abort drawing the set if the user presses escape or clicks the close icon
            InputPoll();
            if (g_window->windowClosed || g_input.keyDowns[KEY_ESC])
                return;
            UpdateWin();
        }
    }

    double endTime = GetRealTime();
    DfFont *font = LoadFontFromMemory(deadfrog_mono_7x13, sizeof(deadfrog_mono_7x13));
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, 10, "Time taken: %.2f sec. Press ESC.", endTime - start_time);

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        InputPoll();
        SleepMillisec(100);
    }
}

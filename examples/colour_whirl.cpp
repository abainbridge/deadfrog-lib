// This example produces a pretty pattern that uses all the colours of a
// palette for exactly one pixel each. It does this by starting by placing a few
// of the colours from the palette as "seed" pixels and then placing the
// remaining colours next to the colour they are "closest" to on the bitmap.

#include "df_font.h"
#include "df_time.h"
#include "df_window.h"
#include "fonts/df_mono.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>


struct PixelLocation { unsigned short x, y; };

// Locations of pixels where new colours can be plotted. This is the boundary on
// the bitmap between coloured pixels we've plotted and the black background.
enum { MAX_LOCATIONS = 100 * 1000 };
static PixelLocation g_availableLocations[MAX_LOCATIONS];
static unsigned g_numAvailableLocations = 0;


inline unsigned ColourDiff(DfColour a, DfColour b) {
    int r = abs(a.r - b.r)/2;
    int g = abs(a.g - b.g);
    int blue = abs(a.b - b.b)/2;
    return r + g + blue;
}


// Calculate diff between specified colour and neighbours of specified location.
unsigned CalcDiff(DfBitmap *bmp, unsigned short x, unsigned short y, DfColour targetColour) {
    unsigned minDiff = UINT_MAX;

    DfColour *row = bmp->pixels + (y-1) * bmp->width + x;

    unsigned diff = ColourDiff(row[0], targetColour);
    if (diff < minDiff)
        minDiff = diff;

    row += bmp->width;
    
    diff = ColourDiff(row[-1], targetColour);
    if (diff < minDiff)
        minDiff = diff;

    diff = ColourDiff(row[1], targetColour);
    if (diff < minDiff)
        minDiff = diff;

    row += bmp->width;

    diff = ColourDiff(row[0], targetColour);
    if (diff < minDiff)
        minDiff = diff;

    return minDiff;
}


unsigned FindBestLocationIndex(DfBitmap *bmp, DfColour c) {
    unsigned best = 0;

    unsigned minDiff = UINT_MAX;
    for (unsigned i = 0; i < g_numAvailableLocations; i++) {
        unsigned short x = g_availableLocations[i].x;
        unsigned short y = g_availableLocations[i].y;
        unsigned diff = CalcDiff(bmp, x, y, c);
        if (diff < minDiff) {
            minDiff = diff;
            best = i;
        }
    }

    return best;
}


static DfColour g_alreadyAddedCol = g_colourBlack;


// Add the specified location to the array of available locations if it isn't already in it
// and isn't the location of a coloured pixel we already plotted.
//
// We avoid searching the entire array by cheating: for every location we add to the
// array, we plot a not quite black pixel on the bitmap. That way we can read the
// colour of the bitmap at the specified location, and if it is not black, we know
// not to add the location to the array.
inline void AddLocationIfNew(DfBitmap *bmp, unsigned short x, unsigned short y) {
    if (GetPix(bmp, x, y).c == g_colourBlack.c) {
        if (x > 1 && x < (bmp->width-1) &&
            y > 1 && y < (bmp->height-1))
        {
			if (g_numAvailableLocations < MAX_LOCATIONS) {
				g_availableLocations[g_numAvailableLocations].x = x;
				g_availableLocations[g_numAvailableLocations].y = y;
				g_numAvailableLocations++;
			}
			PutPix(bmp, x, y, g_alreadyAddedCol);
        }
    }
}


void PlaceColour(DfBitmap *bmp, unsigned short x, unsigned short y, DfColour colour) {
    PutPix(bmp, x, y, colour);

    AddLocationIfNew(bmp, x, y - 1);
    AddLocationIfNew(bmp, x - 1, y);
    AddLocationIfNew(bmp, x + 1, y);
    AddLocationIfNew(bmp, x, y + 1);
}


void ColourWhirlMain() {
    // Setup the window
    int width, height;
    GetDesktopRes(&width, &height);
//    g_defaultWin = CreateWin(width, height, false, "Colour Whirl Example");
    g_window = CreateWin(1000, 1000, WT_WINDOWED_FIXED, "Colour Whirl Example");
    g_defaultFont = LoadFontFromMemory(df_mono_9x18, sizeof(df_mono_9x18));
    BitmapClear(g_window->bmp, g_colourBlack);

    // Create the palette of colours.
    int const maxComponent = 100;
    double const scale = 256.0 / maxComponent;
    DfColour *colours = new DfColour [maxComponent * maxComponent * maxComponent];
    int numColours = 0;
    for (int r = 0; r < maxComponent; r++)
        for (int g = 0; g < maxComponent; g++)
            for (int b = 0; b < maxComponent; b++)
                colours[numColours++] = Colour((int)(r * scale), (int)(g * scale), (int)(b * scale));

    // Shuffle the palette.
    srand((unsigned)(GetRealTime() * 1e6));
    for (int i = 0; i < numColours - 1; i++) {
        uint64_t j0 = rand();
        uint64_t j1 = j0 * (numColours - i);
        uint64_t j2 = j1 / RAND_MAX;
        uint64_t j3 = j2 + i + 1;
        DfColour t = colours[j3];
        colours[j3] = colours[i];
        colours[i] = t;
    }

    double start_time = GetRealTime();

	// Seed the bitmap by manually placing the first few colours.
    g_alreadyAddedCol.b = 1;
    for (int i = 0; i < 5; i++) {
        int x = rand() % g_window->bmp->width;
        int y = rand() % g_window->bmp->height;
        PlaceColour(g_window->bmp, x, y, colours[--numColours]);
    }
 
	// Place all remaining colours.
	numColours--;
    for (; numColours && g_numAvailableLocations; numColours--) {         
        unsigned i = FindBestLocationIndex(g_window->bmp, colours[numColours]);
        unsigned short x = g_availableLocations[i].x;
        unsigned short y = g_availableLocations[i].y;
        if (g_numAvailableLocations > 1)
            g_availableLocations[i] = g_availableLocations[g_numAvailableLocations - 1];
        g_numAvailableLocations--;

        PlaceColour(g_window->bmp, x, y, colours[numColours]);

        // Every so often, copy the bitmap to the screen
        if ((numColours & 0x3ff) == 0) {
			// Abort drawing the set if the user presses escape or clicks the close icon
            InputPoll(g_window);
            if (g_window->windowClosed || g_window->input.keyDowns[KEY_ESC])
                exit(0);
            UpdateWin(g_window);
        }
    }

    double endTime = GetRealTime();
    DrawTextLeft(g_defaultFont, g_colourWhite, g_window->bmp, 10, g_window->bmp->height - 20, 
		"Time taken: %.2f sec. Press ESC.", endTime - start_time);

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_window->input.keys[KEY_ESC]) {
        InputPoll(g_window);
		UpdateWin(g_window);
        SleepMillisec(100);
    }
}

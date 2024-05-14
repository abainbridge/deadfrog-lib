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
#include <string.h>


struct PixelLocation { unsigned short x, y; };

// Locations of pixels where new colours can be plotted. This is the boundary on
// the bitmap between coloured pixels we've plotted and the black background.
enum { MAX_LOCATIONS = 100 * 1000 };
static PixelLocation g_availableLocations[MAX_LOCATIONS];
static unsigned g_numAvailableLocations = 0;


inline unsigned ColourDiff(DfColour a, DfColour b) {
    // Parameter 'a' is the colour we read from the bitmap. It will have one of
    // these possible alpha values: 0=Pixel not yet reached, 1=Pixel is 
    // presently in g_availableLocations, 255=Pixel has its final colour.
    // We are only interested in pixels that have their final colour, so return
    // a large value as the "diff" so that this result isn't chosen as a close
    // match.
    if (a.a <= 1) 
        return 10000;
    int r = abs(a.r - b.r);
    int g = abs(a.g - b.g) * 2;
    int blue = abs(a.b - b.b);
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


// Add the specified location to the array of available locations if it isn't already in it
// and isn't the location of a coloured pixel we already plotted.
//
// We avoid searching the entire array by cheating: for every location we add to the
// array, we plot a not quite black pixel on the bitmap. That way we can read the
// colour of the bitmap at the specified location, and if it is not black, we know
// not to add the location to the array.
inline void AddLocationIfNew(DfBitmap *bmp, unsigned short x, unsigned short y) {
    // Check if pixel has already been visited by looking at alpha value.
    DfColour *pixel = bmp->pixels + bmp->width * y + x;
    if (!pixel->a) {
        if (x > 1 && x < (bmp->width-1) &&
            y > 1 && y < (bmp->height-1))
        {
			if (g_numAvailableLocations < MAX_LOCATIONS) {
				g_availableLocations[g_numAvailableLocations].x = x;
				g_availableLocations[g_numAvailableLocations].y = y;
				g_numAvailableLocations++;
			}

            // Mark this pixel as "already visited" by setting alpha value to 1.
            pixel->a = 1;
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
    
    // Clear the bitmap to black, but not by using the normal BitmapClear() 
    // function because that will set the alpha values to 255. We want them to
    // be zero, because we will use the alpha channel to keep track of which
    // pixels have been visited.
    memset(g_window->bmp->pixels, 0, sizeof(DfColour) * g_window->bmp->width * g_window->bmp->height);

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

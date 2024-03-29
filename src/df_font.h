// This module implements a bitmap font renderer.
// The supported font file format is custom.
// Variable width fonts are supported.
// The renderer currently does not attempt any form of anti-aliasing.
// See docs/fonts.txt for more info.

#pragma once


#include "df_colour.h"
#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _DfBitmap DfBitmap;
typedef struct _Glyph Glyph;


typedef struct _DfFont {
    Glyph *glyphs[256];   // TODO remove the *, so the glyphs are contiguous in memory.
    bool fixedWidth;
    int maxCharWidth;
    int charHeight; // in pixels
} DfFont;

typedef struct _DfFontSource {
    unsigned char const numSizes;
    unsigned char const *pixelWidths; // Max width if not fixed width.
    unsigned char const *pixelHeights;
    unsigned const **dataBlobs;
    unsigned const *dataBlobsSizes; // Number of bytes in each data blob.
} DfFontSource;


// Finds the pixel heights of all the fonts in the file and writes them into
// the results array. If file contains more than 16 fonts, only the first 16
// sizes will be returned. The results array will be sorted, with the lowest
// value first. Return value is the number of items written into the results
// array, or -1 on error.
int ListFontSizesInFile(char const *filename, int result[16]);

// Loads a font from a .dfbf file. Files typically contain multiple font sizes,
// so you must specify the pixel height of the one you want. If the specified
// size is not present, or there is an error, NULL will be returned.
DLL_API DfFont *LoadFontFromFile(char const *filename, int pixHeight);

// Loads a font from a block of data already in memory. Typically this will be
// a font in C code format that was #include'd.
DLL_API DfFont *LoadFontFromMemory(void const *buf, int numBytes);

DLL_API void FontDelete(DfFont *font);

// All the DrawText... functions below return rendered text length in pixels.

// Renders "text" up to the NULL terminator.
DLL_API int DrawTextSimple   (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text);

// Renders "text" up to the NULL terminator or until maxChars is reached - whichever comes first.
DLL_API int DrawTextSimpleLen(DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, int maxChars);

DLL_API int DrawTextLeft     (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);   // Like simple but with variable args
DLL_API int DrawTextRight    (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);   // Like above but with right justify
DLL_API int DrawTextCentre   (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);   // Like above but with centre justify

// Gets the width in pixels of a string in the specified font. Both functions
// assume the string is NULL terminated. The 
DLL_API int GetTextWidth(DfFont *, char const *text);
DLL_API int GetTextWidthNumChars(DfFont *, char const *text, int numChars);


extern DfFont *g_defaultFont;


#ifdef __cplusplus
}
#endif

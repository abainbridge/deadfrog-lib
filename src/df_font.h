// This module implements a somewhat optimized text renderer.
// You create a font by specifying the name of any installed font and
// the point size you want. The font's glyphs will be pre-calculated.
// Variable width fonts are supported.
// The renderer currently does not attempt any form of anti-aliasing.

#pragma once


#include "df_colour.h"
#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


typedef struct _DfBitmap DfBitmap;
class Glyph;


typedef struct _DfFont
{
	Glyph       *glyphs[256];   // TODO remove the *, so the glyphs are contiguous in memory.
	bool		fixedWidth;
	int			maxCharWidth;
	int			charHeight;	// in pixels
} DfFont;


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
// a font in C code format that has been #include'd.
DLL_API DfFont *LoadFontFromMemory(void *buf, int numBytes);

// All the DrawText... functions below return rendered text length in pixels.

// Renders "text" up to the NULL terminator.
DLL_API int DrawTextSimple   (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text);

// Renders "text" up to the NULL terminator or until maxChars is reached - whichever comes first.
DLL_API int DrawTextSimpleLen(DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, int maxChars);

DLL_API int DrawTextLeft     (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRight	 (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentre	 (DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidth	 (DfFont *, char const *text, int len=999999);


extern DfFont *g_defaultFont;


#ifdef __cplusplus
}
#endif

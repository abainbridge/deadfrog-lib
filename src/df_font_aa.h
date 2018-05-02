// This module implements an antialiased text renderer.
// You create a font by specifying the name of any installed font and
// the point size you want. The font's glyphs will be pre-calculated.
// Variable width fonts are supported.

#pragma once


#include "df_colour.h"
#include "df_common.h"

#include "df_sized_glyph.h"


#ifdef __cplusplus
extern "C"
{
#endif


const int MAX_FONT_SIZE = 40;


struct DfBitmap;
class MasterGlyph;


typedef struct _DfFontAa
{
	MasterGlyph *masterGlyphs[256];   // Pointers may be NULL if glyph is blank.
    SizedGlyphSet *sizedGlyphSets[MAX_FONT_SIZE + 1];
	bool	    fixedWidth;
} DfFontAa;


// Size is in pixels. Weight is in range 1 (thin) to 9 (heavy)
DLL_API DfFontAa *FontAaCreate(char const *font_name, int weight);

// Size is height in pixels. By height, I mean the pitch between adjacent rows of text.
DLL_API int DrawTextSimpleAa (DfFontAa *, DfColour c, DfBitmap *, int x, int y, int size, char const *text);      // Returns text length in pixels
DLL_API int DrawTextLeftAa   (DfFontAa *, DfColour c, DfBitmap *, int x, int y, int size, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRightAa	 (DfFontAa *, DfColour c, DfBitmap *, int x, int y, int size, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentreAa (DfFontAa *, DfColour c, DfBitmap *, int x, int y, int size, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidthAa   (DfFontAa *, char const *text, int size, int len=999999);


#ifdef __cplusplus
}
#endif

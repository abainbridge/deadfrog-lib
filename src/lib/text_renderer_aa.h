// This module implements an antialiased text renderer.
// You create a font by specifying the name of any installed font and
// the point size you want. The font's glyphs will be pre-calculated.
// Variable width fonts are supported.

#ifndef TEXT_RENDERER_AA_H
#define TEXT_RENDERER_AA_H


#include "lib/rgba_colour.h"
#include "lib/common.h"

#include "sized_glyph.h"


const int MAX_FONT_SIZE = 40;


struct BitmapRGBA;
class MasterGlyph;


typedef struct
{
	MasterGlyph *masterGlyphs[256];   // Pointers may be NULL if glyph is blank.
    SizedGlyphSet *sizedGlyphSets[MAX_FONT_SIZE + 1];
	bool	    fixedWidth;
} TextRendererAa;


// Size is in pixels. Weight is in range 1 (thin) to 9 (heavy)
DLL_API TextRendererAa *CreateTextRendererAa(char const *font_name, int weight);

// Size is height in pixels. By height, I mean the pitch between adjacent rows of text.
DLL_API int DrawTextSimpleAa (TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, int size, char const *text);      // Returns text length in pixels
DLL_API int DrawTextLeftAa   (TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, int size, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRightAa	 (TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, int size, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentreAa (TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, int size, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidthAa   (TextRendererAa *, char const *text, int size, int len=999999);


#endif

// This module implements an antialiased text renderer.
// You create a font by specifying the name of any installed font and
// the point size you want. The font's glyphs will be pre-calculated.
// Variable width fonts are supported.

#ifndef TEXT_RENDERER_AA_H
#define TEXT_RENDERER_AA_H


#include "lib/rgba_colour.h"
#include "lib/common.h"

struct BitmapRGBA;
class GlyphAa;


typedef struct
{
	GlyphAa     *glyphs[256];
	bool		fixedWidth;
    int         aveCharWidth;
    int			maxCharWidth;
	int			charHeight;	// in pixels
} TextRendererAa;


// Size is in pixels. Weight is in range 1 (thin) to 9 (heavy)
DLL_API TextRendererAa *CreateTextRendererAa(char const *fontname, int size, int weight);

DLL_API int DrawTextSimple		(TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text);       // Returns text length in pixels
DLL_API int DrawTextLeft		(TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRight		(TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentre		(TextRendererAa *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidth		(TextRendererAa *, char const *text, int len=999999);


#endif

// This module implements a somewhat optimized text renderer.
// You create a font by specifying the name of any installed font and
// the point size you want. The font's glyphs will be pre-calculated.
// Variable width fonts are supported.
// The renderer currently does not attempt any form of anti-aliasing.

#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H


#include "lib/rgba_colour.h"
#include "lib/common.h"

struct BitmapRGBA;
class Glyph;


typedef struct
{
	Glyph       *glyphs[256];
	bool		fixedWidth;
	int			maxCharWidth;
	int			charHeight;	// in pixels
} TextRenderer;


DLL_API TextRenderer *CreateTextRenderer(char const *_fontname, int size);

DLL_API int DrawTextSimple		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text);       // Returns text length in pixels
DLL_API int DrawTextLeft		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRight		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentre		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidth		(TextRenderer *, char const *text, int len=999999);


#endif

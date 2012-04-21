#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H


#include "lib/gfx/rgba_colour.h"
#include "lib/common.h"

struct BitmapRGBA;
class Glyph;


struct TextRenderer
{
	Glyph       *glyphs[256];
	bool		fixedWidth;
	int			avgCharWidth;
	int			charHeight;	// in pixels
};


DLL_API TextRenderer *CreateTextRenderer(char const *_fontname, int size, bool fixedPitch);

DLL_API int DrawTextSimple		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, int maxChars = 9999);	// Returns text length in pixels
DLL_API int DrawTextLeft		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRight		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentre		(TextRenderer *, RGBAColour c, BitmapRGBA *, int x, int y, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidth		(TextRenderer *, char const *text, int len=999999);


#endif

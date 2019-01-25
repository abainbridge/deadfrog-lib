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
	Glyph       *glyphs[256];
	bool		fixedWidth;
	int			maxCharWidth;
	int			charHeight;	// in pixels
} DfFont;


// Size is in pixels. Weight is in range 1 (thin) to 9 (heavy)
DLL_API DfFont *FontCreate    (char const *fontname, int size, int weight);

DLL_API int DrawTextSimple		(DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text);       // Returns text length in pixels
DLL_API int DrawTextLeft		(DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);	// Like simple but with variable args
DLL_API int DrawTextRight		(DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);	// Like above but with right justify
DLL_API int DrawTextCentre		(DfFont *, DfColour c, DfBitmap *, int x, int y, char const *text, ...);	// Like above but with centre justify

DLL_API int	GetTextWidth		(DfFont *, char const *text, int len=999999);


extern DfFont *g_defaultFont;


#ifdef __cplusplus
}
#endif

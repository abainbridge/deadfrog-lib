#ifndef INCLUDED_RGB_COLOUR_H
#define INCLUDED_RGB_COLOUR_H


#include "df_common.h"


typedef union
{
    unsigned c;
    struct { unsigned char b, g, r, a; };
} RGBAColour;


DLL_API inline RGBAColour Colour(unsigned r, unsigned g, unsigned b, unsigned a=255)
{
	RGBAColour c;
	c.a = a;
	c.b = b;
	c.g = g;
	c.r = r;
	return c;
}


DLL_API inline RGBAColour RgbaAdd(RGBAColour a, RGBAColour b)
{
    return Colour(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}


DLL_API inline RGBAColour RgbaSubtract(RGBAColour a, RGBAColour b)
{
    return Colour(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
}


DLL_API inline RGBAColour RgbaMultiply(RGBAColour col, float x)
{
    return Colour(col.r * x, col.g * x, col.b * x, col.a * x);
}


DLL_API RGBAColour RgbaAddWithSaturate(RGBAColour a, RGBAColour b);


#if __cplusplus
inline RGBAColour operator+(RGBAColour a, RGBAColour b)
{
    return RgbaAdd(a, b);
}


inline RGBAColour operator-(RGBAColour a, RGBAColour b)
{
    return RgbaSubtract(a, b);
}


inline RGBAColour operator*(RGBAColour col, float x)
{
    return RgbaMultiply(col, x);
}
#endif



DLL_API RGBAColour g_colourBlack;
DLL_API RGBAColour g_colourWhite;


#endif

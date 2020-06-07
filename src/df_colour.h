#pragma once


#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif



typedef union
{
    unsigned c;
    struct { unsigned char b, g, r, a; };
} DfColour;


#ifdef __cplusplus
DLL_API inline DfColour Colour(unsigned r, unsigned g, unsigned b, unsigned a=255)
{
	DfColour c;
	c.a = a;
	c.b = b;
	c.g = g;
	c.r = r;
	return c;
}
#endif

DLL_API inline DfColour RgbaAdd(DfColour a, DfColour b)
{
    return Colour(a.r + b.r, a.g + b.g, a.b + b.b, a.a + b.a);
}


DLL_API inline DfColour RgbaSubtract(DfColour a, DfColour b)
{
    return Colour(a.r - b.r, a.g - b.g, a.b - b.b, a.a - b.a);
}


DLL_API inline DfColour RgbaMultiply(DfColour col, float x)
{
    return Colour(col.r * x, col.g * x, col.b * x, col.a * x);
}


DLL_API DfColour RgbaAddWithSaturate(DfColour a, DfColour b);


#if __cplusplus
inline DfColour operator+(DfColour a, DfColour b)
{
    return RgbaAdd(a, b);
}


inline DfColour operator-(DfColour a, DfColour b)
{
    return RgbaSubtract(a, b);
}


inline DfColour operator*(DfColour col, float x)
{
    return RgbaMultiply(col, x);
}
#endif



DLL_API DfColour g_colourBlack;
DLL_API DfColour g_colourWhite;


#ifdef __cplusplus
}
#endif

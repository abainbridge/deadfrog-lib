#ifndef INCLUDED_RGB_COLOUR_H
#define INCLUDED_RGB_COLOUR_H


#include "lib/common.h"

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


DLL_API RGBAColour g_colourBlack;
DLL_API RGBAColour g_colourWhite;


#endif

#ifndef INCLUDED_RGB_COLOUR_H
#define INCLUDED_RGB_COLOUR_H


#include "lib/common.h"

typedef unsigned RGBAColour;

DLL_API inline unsigned MakeCol(unsigned r, unsigned g, unsigned b, unsigned a)
{
	unsigned c;
	unsigned char *comps = (unsigned char *)&c;
	comps[0] = a;
	comps[1] = b;
	comps[2] = g;
	comps[3] = r;
	return c;
}


#define CompR(c) (((unsigned char *)c)[3])
#define CompG(c) (((unsigned char *)c)[1])
#define CompB(c) (((unsigned char *)c)[2])
#define CompA(c) (((unsigned char *)c)[0])

// void ConvertToHsv();
// void ConvertToRgb();
// 
// void SignedAddWithSaturate(int r, int g, int b);
// void BlendTowards(RGBAColour const &other, float fraction);
// 
// unsigned char GetLuminance(); // Returns a value between 0 and 255

DLL_API extern RGBAColour g_colourBlack;
DLL_API extern RGBAColour g_colourWhite;


#endif

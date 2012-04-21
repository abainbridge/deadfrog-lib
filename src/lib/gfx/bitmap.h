#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H


#include <memory.h>
#include "lib/gfx/rgba_colour.h"
#include "lib/common.h"


struct BitmapRGBA
{
	unsigned width;
	unsigned height;
	RGBAColour *pixels;
	RGBAColour **lines;
};


DLL_API BitmapRGBA *CreateBitmapRGBA(unsigned width, unsigned height);
DLL_API void        DeleteBitmapRGBA(BitmapRGBA *bmp);

DLL_API void        ClearBitmap     (BitmapRGBA *bmp, RGBAColour c);

DLL_API void        DfPutPixel      (BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c);
DLL_API void        PutPixelClipped (BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c);

DLL_API RGBAColour  DfGetPixel      (BitmapRGBA *bmp, unsigned x, unsigned y);
DLL_API RGBAColour  GetPixelClipped (BitmapRGBA *bmp, unsigned x, unsigned y);

DLL_API void        HLine           (BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c);
DLL_API void        VLine           (BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c);
DLL_API void        DrawLine        (BitmapRGBA *bmp, int x1, int y1, int x2, int y2, RGBAColour c);

DLL_API void        RectFill        (BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c);
DLL_API void        RectOutline     (BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c);

DLL_API void        QuickBlit       (BitmapRGBA *destBmp, int x, int y, BitmapRGBA *srcBmp);

// RGBAColour GetInterpolatedPixel(float x, float y);
// 
// void StretchBlit(unsigned srcX,  unsigned srcY,  unsigned srcW,  unsigned srcH, BitmapRGBA *_srcBmp, 
//		    		unsigned destX, unsigned destY, unsigned destW, unsigned destH);







inline void DfPutPixel(BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c)
{
    // if (CompA(colour) == 255)
	bmp->lines[y][x] = c;
	// else
	// 	lines[y][x].BlendTowards(colour, 255.0f / (float)colour.a);
}


#endif

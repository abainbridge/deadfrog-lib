// This module implements a portable bitmap and some simple drawing primitives.
// The bitmap lives entirely in main memory, rather than on the graphics card
// and all the drawing is done by the CPU. It's still reasonably quick though.

#ifndef INCLUDED_BITMAP_H
#define INCLUDED_BITMAP_H


#include "lib/rgba_colour.h"
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

DLL_API void        PutPixUnclipped (BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c);
DLL_API void        PutPix          (BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c);

DLL_API RGBAColour  GetPixUnclipped (BitmapRGBA *bmp, unsigned x, unsigned y);
DLL_API RGBAColour  GetPix          (BitmapRGBA *bmp, unsigned x, unsigned y);

DLL_API void        HLine           (BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c);
DLL_API void        HLineUnclipped  (BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c);
DLL_API void        VLine           (BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c);
DLL_API void        VLineUnclipped  (BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c);
DLL_API void        DrawLine        (BitmapRGBA *bmp, int x1, int y1, int x2, int y2, RGBAColour c);

DLL_API void        RectFill        (BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c);
DLL_API void        RectOutline     (BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c);

// Copies the source bitmap to the destination bitmap, skipping pixels whose source alpha values are 0
DLL_API void        MaskedBlit      (BitmapRGBA *destBmp, int x, int y, BitmapRGBA *srcBmp);

// Copies the source bitmap to the destination bitmap, including pixels whose source alpha values are 0
DLL_API void        QuickBlit       (BitmapRGBA *destBmp, unsigned x, unsigned y, BitmapRGBA *srcBmp);



inline void PutPixUnclipped(BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c)
{
	bmp->lines[y][x] = c;
}


#endif

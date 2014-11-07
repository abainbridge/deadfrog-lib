// This module implements a bitmap data structure and some simple drawing primitives.
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

// Arguments a to d are points, represented as arrays of 2 integers. Curve starts at 'a' and ends at 'd'.
DLL_API void        DrawBezier      (BitmapRGBA *bmp, int const *a, int const *b, int const *c, int const *d, RGBAColour col);

// Calculate the position on a Bezier curve. t is in range 0 to 63356.
DLL_API void        GetBezierPos    (int const *a, int const *b, int const *c, int const *d, int t, int *result);
DLL_API void        GetBezierDir    (int const *a, int const *b, int const *c, int const *d, int t, int *result);

DLL_API void        RectFill        (BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c);
DLL_API void        RectOutline     (BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c);

DLL_API void        CircleOutline   (BitmapRGBA *bmp, int x, int y, unsigned r, RGBAColour c);
DLL_API void        CircleFill      (BitmapRGBA *bmp, int x, int y, unsigned r, RGBAColour c);

// Copies the source bitmap to the destination bitmap, skipping pixels whose source alpha values are 0
DLL_API void        MaskedBlit      (BitmapRGBA *destBmp, int x, int y, BitmapRGBA *srcBmp);

// Copies the source bitmap to the destination bitmap, including pixels whose source alpha values are 0
DLL_API void        QuickBlit       (BitmapRGBA *destBmp, unsigned x, unsigned y, BitmapRGBA *srcBmp);

// Copies the source bitmap to the destination bitmap, scaling the result down by the specified scale factor
DLL_API void        ScaleDownBlit   (BitmapRGBA *destBmp, unsigned x, unsigned y, unsigned scale, BitmapRGBA *srcBmp);



inline void PutPixUnclipped(BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour c)
{
    RGBAColour *pixel = (bmp->pixels + y * bmp->width) + x;
//    RGBAColour *pixel = bmp->lines[y] + x;
    if (c.a == 255)
        *pixel = c;
    else
    {
        unsigned char invA = 255 - c.a;
        pixel->r = (pixel->r * invA + c.r * c.a) / 255;
        pixel->g = (pixel->g * invA + c.g * c.a) / 255;
        pixel->b = (pixel->b * invA + c.b * c.a) / 255;
    }
}


#endif

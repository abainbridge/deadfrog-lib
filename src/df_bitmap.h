// This module implements a bitmap data structure and some simple drawing primitives.
// The bitmap lives entirely in main memory, rather than on the graphics card
// and all the drawing is done by the CPU. It's still reasonably quick though.

#pragma once


#include "df_colour.h"
#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


struct DfBitmap
{
	unsigned width;
	unsigned height;
	DfColour *pixels;
	DfColour **lines;
};


DLL_API DfBitmap   *DfCreateBitmap(unsigned width, unsigned height);
DLL_API void        DfDeleteBitmap(DfBitmap *bmp);

DLL_API void        ClearBitmap     (DfBitmap *bmp, DfColour c);

DLL_API void        PutPixUnclipped (DfBitmap *bmp, unsigned x, unsigned y, DfColour c);
DLL_API void        PutPix          (DfBitmap *bmp, unsigned x, unsigned y, DfColour c);

DLL_API DfColour    GetPixUnclipped (DfBitmap *bmp, unsigned x, unsigned y);
DLL_API DfColour    GetPix          (DfBitmap *bmp, unsigned x, unsigned y);

DLL_API void        HLine           (DfBitmap *bmp, int x, int y, unsigned len, DfColour c);
DLL_API void        HLineUnclipped  (DfBitmap *bmp, int x, int y, unsigned len, DfColour c);
DLL_API void        VLine           (DfBitmap *bmp, int x, int y, unsigned len, DfColour c);
DLL_API void        VLineUnclipped  (DfBitmap *bmp, int x, int y, unsigned len, DfColour c);
DLL_API void        DrawLine        (DfBitmap *bmp, int x1, int y1, int x2, int y2, DfColour c);

// Arguments a to d are points, represented as arrays of 2 integers. Curve starts at 'a' and ends at 'd'.
DLL_API void        DrawBezier      (DfBitmap *bmp, int const *a, int const *b, int const *c, int const *d, DfColour col);

// Calculate the position on a Bezier curve. t is in range 0 to 63356.
DLL_API void        GetBezierPos    (int const *a, int const *b, int const *c, int const *d, int t, int *result);
DLL_API void        GetBezierDir    (int const *a, int const *b, int const *c, int const *d, int t, int *result);

DLL_API void        RectFill        (DfBitmap *bmp, int x, int y, unsigned w, unsigned h, DfColour c);
DLL_API void        RectOutline     (DfBitmap *bmp, int x, int y, unsigned w, unsigned h, DfColour c);

DLL_API void        CircleOutline   (DfBitmap *bmp, int x, int y, unsigned r, DfColour c);
DLL_API void        CircleFill      (DfBitmap *bmp, int x, int y, unsigned r, DfColour c);

DLL_API void        EllipseOutline  (DfBitmap *bmp, int x, int y, unsigned rx, unsigned ry, DfColour c);
DLL_API void        EllipseFill     (DfBitmap *bmp, int x, int y, unsigned rx, unsigned ry, DfColour c);

// Copies the source bitmap to the destination bitmap, skipping pixels whose source alpha values are 0
DLL_API void        MaskedBlit      (DfBitmap *destBmp, int x, int y, DfBitmap *srcBmp);

// Copies the source bitmap to the destination bitmap, including pixels whose source alpha values are 0
DLL_API void        QuickBlit       (DfBitmap *destBmp, unsigned x, unsigned y, DfBitmap *srcBmp);

// Copies the source bitmap to the destination bitmap, scaling the result down by the specified scale factor
DLL_API void        ScaleDownBlit   (DfBitmap *destBmp, unsigned x, unsigned y, unsigned scale, DfBitmap *srcBmp);
DLL_API void        ScaleUpBlit     (DfBitmap *destBmp, unsigned x, unsigned y, unsigned scale, DfBitmap *srcBmp);

DLL_API void 		BitmapDownsample(DfBitmap *src_bmp, DfBitmap *dst_bmp);


inline void PutPixUnclipped(DfBitmap *bmp, unsigned x, unsigned y, DfColour c)
{
    DfColour *pixel = (bmp->pixels + y * bmp->width) + x;
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


#ifdef __cplusplus
}
#endif

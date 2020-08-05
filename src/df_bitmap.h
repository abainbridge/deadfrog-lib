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


typedef struct _DfBitmap
{
	int width;
	int height;

    // Clipping rectangle. Pixel is clipped if and only if:
    //    x < clipLeft || x >= clipRight || y < clipTop || y >= clipBottom
    int clipLeft;
    int clipRight;
    int clipTop;
    int clipBottom;

    DfColour *pixels;   // TODO: remove lines and embed pixels in-line. Then remove BitmapDelete().
	DfColour **lines;
} DfBitmap;


DLL_API DfBitmap   *BitmapCreate(int width, int height);
DLL_API void        BitmapDelete(DfBitmap *bmp);

DLL_API void        SetClipRect     (DfBitmap *bmp, int x, int y, int w, int h);
DLL_API void        ClearClipRect   (DfBitmap *bmp); // Sets bitmap's full size as the clip rect.

DLL_API void        BitmapClear     (DfBitmap *bmp, DfColour c);

DLL_API void        PutPixUnclipped (DfBitmap *bmp, int x, int y, DfColour c);
DLL_API void        PutPix          (DfBitmap *bmp, int x, int y, DfColour c);

DLL_API DfColour    GetPixUnclipped (DfBitmap *bmp, int x, int y);
DLL_API DfColour    GetPix          (DfBitmap *bmp, int x, int y);

DLL_API void        HLine           (DfBitmap *bmp, int x, int y, int len, DfColour c);
DLL_API void        HLineUnclipped  (DfBitmap *bmp, int x, int y, int len, DfColour c);
DLL_API void        VLine           (DfBitmap *bmp, int x, int y, int len, DfColour c);
DLL_API void        VLineUnclipped  (DfBitmap *bmp, int x, int y, int len, DfColour c);
DLL_API void        DrawLine        (DfBitmap *bmp, int x1, int y1, int x2, int y2, DfColour c);

// Arguments a to d are points, represented as arrays of 2 integers. Curve starts at 'a' and ends at 'd'.
DLL_API void        DrawBezier      (DfBitmap *bmp, int const *a, int const *b, int const *c, int const *d, DfColour col);

// Calculate the position on a Bezier curve. t is in range 0 to 63356.
DLL_API void        GetBezierPos    (int const *a, int const *b, int const *c, int const *d, int t, int *result);
DLL_API void        GetBezierDir    (int const *a, int const *b, int const *c, int const *d, int t, int *result);

DLL_API void        RectFill        (DfBitmap *bmp, int x, int y, int w, int h, DfColour c);
DLL_API void        RectOutline     (DfBitmap *bmp, int x, int y, int w, int h, DfColour c);

DLL_API void        CircleOutline   (DfBitmap *bmp, int x, int y, int r, DfColour c);
DLL_API void        CircleFill      (DfBitmap *bmp, int x, int y, int r, DfColour c);

DLL_API void        EllipseOutline  (DfBitmap *bmp, int x, int y, int rx, int ry, DfColour c);
DLL_API void        EllipseFill     (DfBitmap *bmp, int x, int y, int rx, int ry, DfColour c);

// Copies the source bitmap to the destination bitmap, skipping pixels whose source alpha values are 0
DLL_API void        MaskedBlit      (DfBitmap *destBmp, int x, int y, DfBitmap *srcBmp);

// Copies the source bitmap to the destination bitmap, including pixels whose source alpha values are 0
// TODO rename this to just Blit().
DLL_API void        QuickBlit       (DfBitmap *destBmp, int x, int y, DfBitmap *srcBmp);

// Copies the source bitmap to the destination bitmap, scaling the result down by the specified scale factor
DLL_API void        ScaleDownBlit   (DfBitmap *destBmp, int x, int y, int scale, DfBitmap *srcBmp);
DLL_API void        ScaleUpBlit     (DfBitmap *destBmp, int x, int y, int scale, DfBitmap *srcBmp);

DLL_API void 		BitmapDownsample(DfBitmap *src_bmp, DfBitmap *dst_bmp);


inline void PutPixUnclipped(DfBitmap *bmp, int x, int y, DfColour c)
{
    DfColour *pixel = (bmp->pixels + y * bmp->width) + x;
//    DfColour *pixel = bmp->lines[y] + x;
    if (c.a == 255)
        *pixel = c;
    else
    {
        unsigned char invA = 255 - c.a;
        unsigned crb = (c.c & 0xff00ff) * c.a;
        unsigned cg = c.g * c.a;
        unsigned rb = (pixel->c & 0xff00ff) * invA + crb;
        unsigned g = pixel->g * invA + cg;
        pixel->c = rb >> 8;
        pixel->g = g >> 8;
    }
}


#ifdef __cplusplus
}
#endif

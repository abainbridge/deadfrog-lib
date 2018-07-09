#pragma once


#include "df_bitmap.h"


#ifdef __cplusplus
extern "C"
{
#endif


// Stores a Vertex WITH SUBPIXEL ACCURACY in a fixed point format. The bottom
// 4 bits are the fractional part. In other words, x and y values should be
// 16x larger than the pixel coordinates.
// For example, pixel coordinates (0.5, 1.5) is represented by (8, 24).
typedef struct Vertex 
{
    int x, y;
} Vertex;


// Polygon must be convex. Vertex order should be anti-clockwise.
void FillPolygonAa(DfBitmap *bmp, Vertex *verts, int numVerts, DfColour col);


#ifdef __cplusplus
}
#endif

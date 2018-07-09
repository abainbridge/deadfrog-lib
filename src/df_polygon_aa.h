#pragma once


#include "df_bitmap.h"


#ifdef __cplusplus
extern "C"
{
#endif


// Stores a Vertex at 16 times pixel resolution, so that subpixel positions can
// be represented. For example, pixel coordinates (0.5, 1.5) is represented by 
// (8, 24).
typedef struct Vertex 
{
    int x, y;
} Vertex;


// Polygon must be convex. Vertex order should be anti-clockwise.
void FillPolygonAa(DfBitmap *bmp, Vertex *verts, int numVerts, DfColour col);


#ifdef __cplusplus
}
#endif

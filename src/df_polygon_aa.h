#pragma once


#include "df_bitmap.h"


#ifdef __cplusplus
extern "C"
{
#endif


// Stores a Vertex at 16 times pixel resolution, so that subpixel positions can
// be represented. For example, pixel coordinate (0.5, 1.5) is represented by 
// (8, 24).
typedef struct DfVertex {
    int x, y;
} DfVertex;


// Polygon must be convex. Vertex order should be anti-clockwise.
void FillPolygonAa(DfBitmap *bmp, DfVertex *verts, int numVerts, DfColour col);


#ifdef __cplusplus
}
#endif

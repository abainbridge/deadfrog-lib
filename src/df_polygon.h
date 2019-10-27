// This code originally came from Michael Abrash's Zen of Graphics, 
// 2nd edition, chapter 23.

#pragma once


#include "df_bitmap.h"


#ifdef __cplusplus
extern "C"
{
#endif


enum { MAX_POLY_VERTICES = 20 };


// Describes a single point (used for a single vertex)
typedef struct 
{
    short x;
    short y;
} PolyVert;


// Describes a series of points (used to store a list of vertices that
// describe a polygon; each vertex is assumed to connect to the two
// adjacent vertices, and the last vertex is assumed to connect to the
// first)
typedef struct 
{
    int numPoints;
    PolyVert points[MAX_POLY_VERTICES];
} PolyVertList;


int FillConvexPolygon(DfBitmap *bmp, PolyVertList *vertList, DfColour col,
                      int xOffset, int yOffset);


#ifdef __cplusplus
}
#endif

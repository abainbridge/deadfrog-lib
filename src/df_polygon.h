// This code originally came from Michael Abrash's Zen of Graphics, 
// 2nd edition, chapter 23.

#ifndef INCLUDED_POLYGON_H
#define INCLUDED_POLYGON_H

#include "df_bitmap.h"


// Describes a single point (used for a single vertex)
typedef struct 
{
    int x;
    int y;
} Point;


// Describes a series of points (used to store a list of vertices that
// describe a polygon; each vertex is assumed to connect to the two
// adjacent vertices, and the last vertex is assumed to connect to the
// first)
typedef struct 
{
    int numPoints;
    Point *points;
} PointListHeader;


int FillConvexPolygon(DfBitmap *bmp, PointListHeader *vertexList, DfColour col,
                      int xOffset, int yOffset);


#endif

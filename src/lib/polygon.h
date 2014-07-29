#ifndef INCLUDED_POLYGON_H
#define INCLUDED_POLYGON_H

#include "bitmap.h"

// Describes a single point (used for a single vertex)
typedef struct {
   int X;
   int Y;
} Point;

// Describes a series of points (used to store a list of vertices that
// describe a polygon; each vertex is assumed to connect to the two
// adjacent vertices, and the last vertex is assumed to connect to the
// first)
typedef struct {
   int Length;          // # of points
   Point *PointPtr;     // pointer to list of points
} PointListHeader;

int FillConvexPolygon(BitmapRGBA *bmp, PointListHeader * VertexList, RGBAColour Color,
                      int XOffset, int YOffset);

#endif

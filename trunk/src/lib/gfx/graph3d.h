#ifndef INCLUDED_GRAPH3D_H
#define INCLUDED_GRAPH3D_H


#include "lib/gfx/rgba_colour.h"
#include "lib/common.h"


struct BitmapRGBA;
struct Graph3d;


DLL_API Graph3d *CreateGraph3d();
DLL_API void Graph3dAddPoint(Graph3d *g, float x, float y, float z);
DLL_API void Graph3dRender(Graph3d *graph, BitmapRGBA *bmp, float cx, float cy, 
                           float dist, float zoom, RGBAColour col, float rotZ);


#endif

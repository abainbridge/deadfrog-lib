#include "graph3d.h"
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include "lib/gfx/bitmap.h"


struct Vector2
{
    float x, y;
};


struct Vector3
{
    float x, y, z;
};


struct Point
{
    Vector3 src;        // Original co-ords passed to AddPoint
    Vector3 dst;        // Co-ords after transforms
    RGBAColour col;
};


struct Graph3d
{
    Point *points;
    int numPoints;
    int maxPoints;
    Vector3 maxPoint;   // Holds the highest x, y and z values seen in all the points
    Vector3 minPoint;

    Vector3 verts[8];
    int lines[24];
};


DLL_API Graph3d *CreateGraph3d()
{
    Graph3d *g = new Graph3d;
    memset(g, 0, sizeof(Graph3d));

    g->points = new Point [16];
    g->numPoints = 0;
    g->maxPoints = 16;

    Vector3 verts[] = {{-128,-128,-128}, {128,-128,-128}, {-128,128,-128}, 
                       {128,128,-128}, {-128,-128,128}, {128,-128,128}, 
                       {-128,128,128}, {128,128,128}};
    int lines[] = {0,1, 1,3, 3,2, 2,0,
                   4,5, 5,7, 7,6, 6,4, 
                   0,4, 1,5, 2,6, 3,7};

    memcpy(g->verts, verts, sizeof(g->verts));
    memcpy(g->lines, lines, sizeof(g->lines));

    return g;
}


DLL_API void Graph3dAddPoint(Graph3d *g, float x, float y, float z, RGBAColour col)
{
    if (g->numPoints == g->maxPoints)
    {
        Point *newPoints = new Point [g->maxPoints * 2];
        memcpy(newPoints, g->points, sizeof(Point) * g->maxPoints);
        g->maxPoints *= 2;
        delete [] g->points;
        g->points = newPoints;
    }

    Point *p = &g->points[g->numPoints];
    p->src.x = x;
    p->src.y = y;
    p->src.z = z;
    p->col = col;
    g->numPoints++;

    g->maxPoint.x = __max(x, g->maxPoint.x);
    g->maxPoint.y = __max(y, g->maxPoint.y);
    g->maxPoint.z = __max(z, g->maxPoint.z);

    g->minPoint.x = __min(x, g->minPoint.x);
    g->minPoint.y = __min(y, g->minPoint.y);
    g->minPoint.z = __min(z, g->minPoint.z);
}


static void Rotate(Graph3d *g, float rotX, float rotZ)
{
    float cx = cosf(rotX);
    float sx = sinf(rotX);
    float cz = cosf(rotZ);
    float sz = sinf(rotZ);

    // Transform points of data set
    for (int i = 0; i < g->numPoints; i++)
    {
        Vector3 *p = &g->points[i].src;
        Vector3 *q = &g->points[i].dst;

        // Rotate around z axis
        q->x = p->x * cz + p->z * sz;
        q->z = p->x * sz - p->z * cz;

        // Rotate around x axis
        q->y = p->y * cx + q->z * sx;
        q->z = q->z * cx - p->y * sx;
    }

    // Form bounding box
    g->verts[0].x = g->minPoint.x;
    g->verts[1].x = g->maxPoint.x;
    g->verts[2].x = g->minPoint.x;
    g->verts[3].x = g->maxPoint.x;
    g->verts[4].x = g->minPoint.x;
    g->verts[5].x = g->maxPoint.x;
    g->verts[6].x = g->minPoint.x;
    g->verts[7].x = g->maxPoint.x;

    g->verts[0].y = g->minPoint.y;
    g->verts[1].y = g->minPoint.y;
    g->verts[2].y = g->maxPoint.y;
    g->verts[3].y = g->maxPoint.y;
    g->verts[4].y = g->minPoint.y;
    g->verts[5].y = g->minPoint.y;
    g->verts[6].y = g->maxPoint.y;
    g->verts[7].y = g->maxPoint.y;

    g->verts[0].z = g->minPoint.z;
    g->verts[1].z = g->minPoint.z;
    g->verts[2].z = g->minPoint.z;
    g->verts[3].z = g->minPoint.z;
    g->verts[4].z = g->maxPoint.z;
    g->verts[5].z = g->maxPoint.z;
    g->verts[6].z = g->maxPoint.z;
    g->verts[7].z = g->maxPoint.z;

    // Transform points of bounding box
    for (int i = 0; i < 8; i++)
    {
        Vector3 *p = &g->verts[i];

        float x = p->x * cz + p->z * sz;
        p->z =    p->x * sz - p->z * cz;
        p->x = x;

        // Rotate around x axis
        float y = p->y;
        p->y = p->y * cx + p->z * sx;
        p->z = p->z * cx - y * sx;
    }
}


static inline Vector2 ProjectPoint(Vector3 a, float cx, float cy, float dist, float zoom)
{
    Vector2 b;
    float z = a.z + dist;
    b.x = (a.x / z) * zoom + cx;
    b.y = (a.y / z) * zoom + cy;
    return b;
}


DLL_API void Graph3dRender(Graph3d *graph, BitmapRGBA *bmp, float cx, float cy, 
                           float dist, float zoom, RGBAColour col,
                           float rotX, float rotZ)
{
    Rotate(graph, rotX, rotZ);

    for (int i = 0; i < graph->numPoints; i++)
    {
        Vector2 p = ProjectPoint(graph->points[i].dst, cx, cy, dist, zoom);
        PutPixelClipped(bmp, p.x, p.y, graph->points[i].col);
    }

    for (int i = 0; i < 12; i++)
    {
        int idx1 = graph->lines[i*2];
        int idx2 = graph->lines[i*2+1];
        Vector2 a = ProjectPoint(graph->verts[idx1], cx, cy, dist, zoom);
        Vector2 b = ProjectPoint(graph->verts[idx2], cx, cy, dist, zoom);
        DrawLine(bmp, a.x, a.y, b.x, b.y, col);
    }
}

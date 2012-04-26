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


struct SortedPoint
{
    Vector3 pos;
    RGBAColour col;
};


#define NUM_DEPTH_BINS 64
struct Graph3d
{
    Point *points;      // First 8 points hold the corners of the cube
    int numPoints;
    int maxPoints;
    
    float minZ;
    float maxZ;
    SortedPoint *sortedPoints;  // Only approximately sorted - good enough, fast and simple

    Vector3 maxPoint;   // Holds the highest x, y and z values seen in all the points
    Vector3 minPoint;
};


DLL_API Graph3d *CreateGraph3d()
{
    Graph3d *g = new Graph3d;
    memset(g, 0, sizeof(Graph3d));

    g->points = new Point [16];
    g->numPoints = 0;
    g->maxPoints = 16;

    g->sortedPoints = new SortedPoint [16];

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

        delete [] g->sortedPoints;
        g->sortedPoints = new SortedPoint [g->maxPoints];
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
    // Form bounding box
    for (int i = 0; i < 8; i++)
    {
        g->points[i].src.x = g->minPoint.x;
        g->points[i].src.y = g->minPoint.y;
        g->points[i].src.z = g->minPoint.z;

        if (i & 1)
            g->points[i].src.x = g->maxPoint.x;

        if (i & 2)
            g->points[i].src.y = g->maxPoint.y;

        if (i & 4)
            g->points[i].src.z = g->maxPoint.z;
    }

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
}


static void DepthSort(Graph3d *graph)
{
    // Find the nearest and furthest point
    for (int i = 8; i < graph->numPoints; i++)
    {
        graph->minZ = __min(graph->minZ, graph->points[i].dst.z);
        graph->maxZ = __max(graph->maxZ, graph->points[i].dst.z);
    }

    float depthFactor = 15.0f / (graph->maxZ - graph->minZ);

    // Count the number of points in each depth bin
    int binCounts[NUM_DEPTH_BINS] = { 0 };      // Number of points in each bin
    for (int i = 8; i < graph->numPoints; i++)
    {
        int binIdx = NUM_DEPTH_BINS - int((graph->points[i].dst.z - graph->minZ) * depthFactor) - 1;
        binCounts[binIdx]++;
    }

    // Setup the depth bins to point to the correct amount of space in depthBinBuf
    int pointCount = 8;
    SortedPoint *depthBins[NUM_DEPTH_BINS];  // Indices into points, grouped by depth
    for (int i = 0; i < NUM_DEPTH_BINS; i++)
    {
        depthBins[i] = &graph->sortedPoints[pointCount];
        pointCount += binCounts[i];
    }

    // Put the points in the appropriate bin
    memset(binCounts, 0, sizeof(binCounts));
    for (int i = 8; i < graph->numPoints; i++)
    {
        // Find which bin this point belongs in
        int binIdx = NUM_DEPTH_BINS - int((graph->points[i].dst.z - graph->minZ) * depthFactor) - 1;
        
        // Copy the view space coords from the unsorted array of points into the 
        // pseudo-sorted array.
        SortedPoint *thisBin = depthBins[binIdx];
        int idxIntoBin = binCounts[binIdx];
        memcpy(&thisBin[idxIntoBin].pos, &graph->points[i].dst, sizeof(Vector3));
        thisBin[idxIntoBin].col = graph->points[i].col;
        
        binCounts[binIdx]++;
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
    DepthSort(graph);

    // Draw the points
    for (int i = 8; i < graph->numPoints; i++)
    {
        Vector2 p = ProjectPoint(graph->sortedPoints[i].pos, cx, cy, dist, zoom);
        PutPixelClipped(bmp, p.x, p.y, graph->sortedPoints[i].col);
    }

    int boundingLines[] = {0,1, 1,3, 3,2, 2,0,
                           4,5, 5,7, 7,6, 6,4, 
                           0,4, 1,5, 2,6, 3,7};

    // Draw the bounding cube
    for (int i = 0; i < 12; i++)
    {
        int idx1 = boundingLines[i*2];
        int idx2 = boundingLines[i*2+1];
        Vector2 a = ProjectPoint(graph->points[idx1].dst, cx, cy, dist, zoom);
        Vector2 b = ProjectPoint(graph->points[idx2].dst, cx, cy, dist, zoom);
        DrawLine(bmp, a.x, a.y, b.x, b.y, col);
    }
}

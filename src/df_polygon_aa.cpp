// Based on:
// Fast Anti-Aliasing Polygon Scan Conversion by Jack Morrison from "Graphics
// Gems", Academic Press, 1990.

// This code renders a polygon, computing subpixel coverage at
// 8 times Y and 16 times X display resolution for anti-aliasing.


#include "df_polygon_aa.h"

#include "df_bitmap.h"
#include "df_common.h"

#include <limits.h>
#include <math.h>


static const int SUBXRES = 16;      // subpixel X resolution per pixel 
static const int SUBYRES = 8;       // subpixel Y resolution per scanline 

#define MOD_Y_RES(y)   ((y) & 7)    // subpixel Y modulo 


struct SubpixelRowExtents {
    int left, right;
};


// Array to store the start and end X values for each row of subpixels in the
// current scanline.
static SubpixelRowExtents subpixelRowExtents[SUBYRES];

// Min and max X values of subpixels in the current pixel. Can be found by
// searching subpixelRowExtents. Only stored as an optimization.
static int leftMin, rightMax;


// Compute number of subpixels covered by polygon at current pixel.
// x is left subpixel of pixel.
static int ComputePixelCoverage(int x)
{
    static const int MAX_AREA = SUBXRES * SUBYRES;
    int area = 0;
    x *= SUBXRES;
    int xr = x + SUBXRES - 1;   // Right-most subpixel of pixel.

    for (int y = 0; y < SUBYRES; y++) {
        // Calc covered area for current subpixel y 
        int partialArea = IntMin(subpixelRowExtents[y].right, xr) - 
            IntMax(subpixelRowExtents[y].left, x) + 1;
        if (partialArea > 0) {
            area += partialArea;
        }
    }

    return 255 * area / MAX_AREA;
}


static void RenderScanline(DfBitmap *bmp, int y, DfColour col)
{
    DfColour tmp = col;
    int x;
    for (x = leftMin / SUBXRES; x <= (rightMax / SUBXRES); x++) {
        int coverage = ComputePixelCoverage(x);
        if (coverage == 255) {
            break;
        }
        tmp.a = (col.a * coverage) >> 8;
        PutPix(bmp, x, y, tmp);
    }
    int x2;
    for (x2 = (rightMax / SUBXRES); x2 > x; x2--) {
        int coverage = ComputePixelCoverage(x2);
        if (coverage == 255) {
            break;
        }
        tmp.a = (col.a * coverage) >> 8;
        PutPix(bmp, x2, y, tmp);
    }

    if (x2 >= x) {
        HLine(bmp, x, y, x2 - x + 1, col);
    }
}


bool IsConvexAndAnticlockwise(DfVertex *verts, int numVerts) {
    for (int a = 0; a < numVerts; a++) {
        int b = (a + 1) % numVerts;
        int c = (a + 2) % numVerts;

        int abx = verts[b].x - verts[a].x;
        int aby = verts[b].y - verts[a].y;
        int bcx = verts[c].x - verts[b].x;
        int bcy = verts[c].y - verts[b].y;

        int crossProduct = abx * bcy - bcx * aby;
        if (crossProduct > 0)
            return false;
    }

    return true;
}


void FillPolygonAa(DfBitmap *bmp, DfVertex *verts, int numVerts, DfColour col)
{
    if (!IsConvexAndAnticlockwise(verts, numVerts))
        return;

    // Convert the verts passed in into the format we use internally,
    // find the max vertex y value and the vertex with minimum y.
    DfVertex *vertLeft = verts;
    int maxY = -1;
    for (int i = 0; i < numVerts; i++) {
        verts[i].y /= SUBXRES / SUBYRES;
        if (verts[i].y < vertLeft->y) {
            vertLeft = &verts[i];
        }
        maxY = IntMax(maxY, verts[i].y);
    }
    DfVertex *endVert = &verts[numVerts - 1];

    // Initialize scanning edges.
    DfVertex *nextVertLeft, *vertRight, *nextVertRight;
    vertRight = nextVertRight = nextVertLeft = vertLeft;

#define NEXT_LEFT_EDGE() \
    vertLeft = nextVertLeft; \
    nextVertLeft++; \
    if (nextVertLeft > endVert) nextVertLeft = verts; // Wrap.

#define NEXT_RIGHT_EDGE() \
    vertRight = nextVertRight; \
    nextVertRight--; \
    if (nextVertRight < verts) nextVertRight = endVert; // Wrap.

    // Skip any initial horizontal edges because they would cause a divide by
    // zero in the slope calculation. We know once we've got over the initial
    // horizontal edges that there cannot be anymore in a convex poly, other
    // than those that would form the bottom of the poly. We'll never
    // encounter those either because the main loop will terminate just before
    // we get to those (because y will have become equal to maxY).
    while (vertLeft->y == nextVertLeft->y) {
        NEXT_LEFT_EDGE();
    }
    while (vertRight->y == nextVertRight->y) {
        NEXT_RIGHT_EDGE();
    }

    // Initialize the extents for each row of subpixels.
    for (int i = 0; i < SUBYRES; i++) {
        subpixelRowExtents[i].left = -1;
        subpixelRowExtents[i].right = -1;
    }

    leftMin = INT_MAX;
    rightMax = -1;
    int leftSlope = ((nextVertLeft->x - vertLeft->x) << 16) / (nextVertLeft->y - vertLeft->y);
    int rightSlope = ((nextVertRight->x - vertRight->x) << 16) / (nextVertRight->y - vertRight->y);

    // Consider each row of subpixels from top to bottom.
    for (int y = vertLeft->y; y < maxY; y++) {
        // Have we reached the end of the left hand edge we are following?
        if (y == nextVertLeft->y) {
            NEXT_LEFT_EDGE();
            leftSlope = ((nextVertLeft->x - vertLeft->x) << 16) / (nextVertLeft->y - vertLeft->y);
        }

        // Have we reached the end of the right hand edge we are following?
        if (y == nextVertRight->y) {
            NEXT_RIGHT_EDGE();
            rightSlope = ((nextVertRight->x - vertRight->x) << 16) / (nextVertRight->y - vertRight->y);
        }

        // Interpolate sub-pixel x endpoints at this y and update extremes.
        
        SubpixelRowExtents *sre = &subpixelRowExtents[MOD_Y_RES(y)];
        sre->left = vertLeft->x + (((y - vertLeft->y) * leftSlope) >> 16);
        leftMin = IntMin(leftMin, sre->left); 

        sre->right = vertRight->x + (((y - vertRight->y) * rightSlope) >> 16);
        rightMax = IntMax(rightMax, sre->right);

        // Is this the last row of subpixels for this scanline?
        if (MOD_Y_RES(y) == SUBYRES - 1) { 
            RenderScanline(bmp, y / SUBYRES, col);
            leftMin = INT_MAX;
            rightMax = -1;
        }
    }

    // Mark remaining subpixel rows as empty.
    for (int yy = MOD_Y_RES(maxY); MOD_Y_RES(yy); yy++) {
        subpixelRowExtents[yy].left = subpixelRowExtents[yy].right = -1;
    }
    RenderScanline(bmp, maxY / SUBYRES, col);

    // Convert the verts back into the format the caller uses. 
    for (int i = 0; i < numVerts; i++) {
        verts[i].y *= SUBXRES / SUBYRES;
    }
}

// Based on:
// Fast Anti-Aliasing Polygon Scan Conversion by Jack Morrison from "Graphics
// Gems", Academic Press, 1990.

// This code renders a polygon, computing subpixel coverage at
// 8 times Y and 16 times X display resolution for anti-aliasing.


#include "df_polygon_aa.h"

#include "df_bitmap.h"
#include "df_common.h"

#include <algorithm>
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
static int leftMin, leftMax, rightMin, rightMax;

static int LERP(double alpha, int a, int b) {
    return a + (b - a) * alpha;
}

// Compute number of subpixels covered by polygon at current pixel
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
        if (partialArea > 0)
            area += partialArea;
    }
    return 255 * area / MAX_AREA;
}

static void RenderScanline(DfBitmap *bmp, int y, DfColour col)
{
    int solidRunStartX = leftMax / SUBXRES;
    int solidRunEndX = rightMin / SUBXRES - 1;
    if (solidRunStartX < solidRunEndX) {
        for (int x = leftMin / SUBXRES; x <= solidRunStartX; x++) {
            col.a = ComputePixelCoverage(x);
            PutPix(bmp, x, y, col);
        }
		col.a = 255;
        HLine(bmp, solidRunStartX + 1, y, (solidRunEndX - solidRunStartX), col);
        for (int x = solidRunEndX; x <= (rightMax / SUBXRES); x++) {
            col.a = ComputePixelCoverage(x);
            PutPix(bmp, x, y, col);
        }
    }
    else {
        for (int x = leftMin / SUBXRES; x <= (rightMax / SUBXRES); x++) {
            col.a = ComputePixelCoverage(x);
            PutPix(bmp, x, y, col);
        }
    }
}

void FillPolygonAa(DfBitmap *bmp, Vertex *verts, int numVerts, DfColour col)
{
    // Convert the verts passed in into the format we use internally. 
    for (int i = 0; i < numVerts; i++) {
        verts[i].y /= SUBXRES / SUBYRES;
    }

    // Find the vertex with minimum y.
    Vertex *vertLeft = verts;
    for (int i = 1; i < numVerts; i++) {
        if (verts[i].y < vertLeft->y) {
            vertLeft = &verts[i];
        }
    }
    Vertex *endVert = &verts[numVerts - 1];

    // Initialize scanning edges.
    Vertex *nextVertLeft, *vertRight, *nextVertRight;
    vertRight = nextVertRight = nextVertLeft = vertLeft;

    // Prepare bottom of initial scanline - no coverage by polygon 
    for (int i = 0; i < SUBYRES; i++) {
        subpixelRowExtents[i].left = -1;
        subpixelRowExtents[i].right = -1;
    }
    leftMin = rightMin = INT_MAX;
    leftMax = rightMax = -1;
    double alphaStepLeft = 0;
    double alphaStepRight = 0;

    // Consider for each row of subpixels from top to bottom.
    for (int y = vertLeft->y; ; y++) {
        // Have we reached the end of the left hand edge we are following?
        while (y == nextVertLeft->y) {
            nextVertLeft = (vertLeft=nextVertLeft) + 1;  // advance 
            if (nextVertLeft > endVert)            // wraparound
                nextVertLeft = verts;
            if (nextVertLeft == vertRight)      // all y's same?
                goto done;                      // (null polygon)
            alphaStepLeft = 1.0 / (nextVertLeft->y - vertLeft->y);
        }

        // Have we reached the end of the right hand edge we are following?
        while (y == nextVertRight->y) {
            nextVertRight = (vertRight=nextVertRight) - 1;
            if (nextVertRight < verts)           // wraparound
                nextVertRight = endVert;
            alphaStepRight = 1.0 / (nextVertRight->y - vertRight->y);
        }

        if (y > nextVertLeft->y || y > nextVertRight->y) {
            // Done. Mark remaining subpixel rows as empty.
            for (; MOD_Y_RES(y); y++) {
                subpixelRowExtents[MOD_Y_RES(y)].left = subpixelRowExtents[MOD_Y_RES(y)].right = -1;
            }
            RenderScanline(bmp, y / SUBYRES, col);
            goto done;
        }

        // Interpolate sub-pixel x endpoints at this y and update extremes.
        
        SubpixelRowExtents *sre = &subpixelRowExtents[MOD_Y_RES(y)];
        double alpha = (double)(y - vertLeft->y) * alphaStepLeft;
        sre->left = LERP(alpha, vertLeft->x, nextVertLeft->x);
        leftMin = IntMin(leftMin, sre->left);
        leftMax = IntMax(leftMax, sre->left);

        alpha = (double)(y - vertRight->y) * alphaStepRight;
        sre->right = LERP(alpha, vertRight->x, nextVertRight->x);
        rightMin = IntMin(rightMin, sre->right);
        rightMax = IntMax(rightMax, sre->right);

        // Is this the last row of subpixels for this scanline?
        if (MOD_Y_RES(y) == SUBYRES - 1) { 
            RenderScanline(bmp, y / SUBYRES, col);
            leftMin = rightMin = INT_MAX;
            leftMax = rightMax = -1;
        }
    }

done:
    // Convert the verts back into the format the caller uses. 
    for (int i = 0; i < numVerts; i++) {
        verts[i].y *= SUBXRES / SUBYRES;
    }
}

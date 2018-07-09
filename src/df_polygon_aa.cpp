// Based on:
// Fast Anti-Aliasing Polygon Scan Conversion by Jack Morrison from "Graphics
// Gems", Academic Press, 1990.

// This code renders a polygon, computing subpixel coverage at
// 8 times Y and 16 times X display resolution for anti-aliasing.


#include "df_polygon_aa.h"

#include "df_bitmap.h"
#include "df_common.h"

#include <algorithm>
#include <math.h>


static const int SUBYRES = 8;       // subpixel Y resolution per scanline 
static const int SUBXRES = 16;      // subpixel X resolution per pixel 
static const int MAX_AREA = SUBXRES * SUBYRES;

#define MOD_Y_RES(y)   ((y) & 7)    // subpixel Y modulo 
#define MAX_X 0x7FFF                // subpixel X beyond right edge 


struct SubPixelScanlineExtents 
{
    int xLeft, xRight;
};

static SubPixelScanlineExtents subpixelScanlineExtents[SUBYRES];
static int xLmin, xLmax;       // subpixel x extremes for scanline 
static int xRmax, xRmin;       // (for optimization shortcut) 

static int LERP(double alpha, int a, int b) {
    return a + (b - a) * alpha;
}

// Compute number of subpixels covered by polygon at current pixel
// x is left subpixel of pixel.
static int ComputePixelCoverage(int x)
{
    int area = 0;           // total covered area 
    int xr = x + SUBXRES - 1;   // right subpixel of pixel 

    for (int y = 0; y < SUBYRES; y++) {
        // Calc covered area for current subpixel y 
        int partialArea = IntMin(subpixelScanlineExtents[y].xRight, xr) - 
            IntMax(subpixelScanlineExtents[y].xLeft, x) + 1;
        if (partialArea > 0)
            area += partialArea;
    }
    return 255 * area / MAX_AREA;
}

static void RenderScanline(DfBitmap *bmp, int y, DfColour col)
{
	int MASK = ~(SUBXRES - 1);
    int solidRunStartX = (xLmax & MASK) + SUBXRES;
    int solidRunEndX = (xRmin & MASK) - SUBXRES;
    if (solidRunStartX < solidRunEndX) {
        for (int x = xLmin & MASK; x <= solidRunStartX; x += SUBXRES) {
            col.a = ComputePixelCoverage(x);
            PutPix(bmp, x / SUBXRES, y, col);
        }
		col.a = 255;
        HLine(bmp, solidRunStartX/SUBXRES, y, 
            (solidRunEndX - solidRunStartX) / SUBXRES + 1, col);
        for (int x = solidRunEndX; x <= xRmax; x += SUBXRES) {
            col.a = ComputePixelCoverage(x);
            PutPix(bmp, x / SUBXRES, y, col);
        }
    }
    else {
        for (int x = xLmin & MASK; x <= xRmax; x += SUBXRES) {
            col.a = ComputePixelCoverage(x);
            PutPix(bmp, x / SUBXRES, y, col);
        }
    }
}

void FillPolygonAa(DfBitmap *bmp, Vertex *verts, int numVerts, DfColour col)
{
    // Convert the verts passed in into the format we use internally. 
    for (int i = 0; i < numVerts; i++)
        verts[i].y /= SUBXRES / SUBYRES;

    // Find the vertex with minimum y.
    Vertex *vertLeft = verts;
    for (int i = 1; i < numVerts; i++) {
        if (verts[i].y < vertLeft->y)
            vertLeft = &verts[i];
    }
    Vertex *endVert = &verts[numVerts - 1];

    // initialize scanning edges 
    Vertex *nextVertLeft;
    Vertex *vertRight, *nextVertRight;
    vertRight = nextVertRight = nextVertLeft = vertLeft;

    // prepare bottom of initial scanline - no coverage by polygon 
    for (int i = 0; i < SUBYRES; i++) {
        subpixelScanlineExtents[i].xLeft = -1;
        subpixelScanlineExtents[i].xRight = -1;
    }
    xLmin = xRmin = MAX_X;
    xLmax = xRmax = -1;

    // Scan convert for each subpixel from top to bottom.
    for (int y = vertLeft->y; ; y++) {
        // Have we reached the end of the left hand edge we are following?
        while (y == nextVertLeft->y) {
            nextVertLeft = (vertLeft=nextVertLeft) + 1;  // advance 
            if (nextVertLeft > endVert)            // (wraparound) 
                nextVertLeft = verts;
            if (nextVertLeft == vertRight)    // all y's same?
                goto done;                 // (null polygon)  
        }

        // Have we reached the end of the right hand edge we are following?
        while (y == nextVertRight->y) {
            nextVertRight = (vertRight=nextVertRight) - 1;
            if (nextVertRight < verts)           // (wraparound) 
                nextVertRight = endVert;
        }

        if (y > nextVertLeft->y || y > nextVertRight->y) {
            // done, mark uncovered part of last scanline 
            for (; MOD_Y_RES(y); y++)
                subpixelScanlineExtents[MOD_Y_RES(y)].xLeft = subpixelScanlineExtents[MOD_Y_RES(y)].xRight = -1;
            RenderScanline(bmp, y / SUBYRES, col);
            goto done;
        }

        // Interpolate sub-pixel x endpoints at this y,
        // and update extremes for pixel coherence optimization
        
        SubPixelScanlineExtents *spse = &subpixelScanlineExtents[MOD_Y_RES(y)];
        double alpha = (double)(y - vertLeft->y) / (nextVertLeft->y - vertLeft->y);
        spse->xLeft = LERP(alpha, vertLeft->x, nextVertLeft->x);
        if (spse->xLeft < xLmin)
            xLmin = spse->xLeft;
        if (spse->xLeft > xLmax)
            xLmax = spse->xLeft;

        alpha = (double)(y - vertRight->y) / (nextVertRight->y - vertRight->y);
        spse->xRight = LERP(alpha, vertRight->x, nextVertRight->x);
        if (spse->xRight < xRmin)
            xRmin = spse->xRight;
        if (spse->xRight > xRmax)
            xRmax = spse->xRight;

        if (MOD_Y_RES(y) == SUBYRES - 1) {   // end of scanline 
            RenderScanline(bmp, y / SUBYRES, col);
            xLmin = xRmin = MAX_X;      // reset extremes 
            xLmax = xRmax = -1;
        }
    }

done:
    // Convert the verts back into the format the caller uses. 
    for (int i = 0; i < numVerts; i++)
        verts[i].y *= SUBXRES / SUBYRES;
}

// This code originally came from Michael Abrash's Zen of Graphics, 
// 2nd edition, chapter 23.

// Color-fills a convex polygon. All vertices are offset by (XOffset,
// YOffset). "Convex" means that every horizontal line drawn through
// the polygon at any point would cross exactly two active edges
// (neither horizontal lines nor zero-length edges count as active
// edges; both are acceptable anywhere in the polygon), and that the
// right & left edges never cross. (It's OK for them to touch, though,
// so long as the right edge never crosses over to the left of the
// left edge.) Nonconvex polygons won't be drawn properly. Returns 1
// for success, 0 if memory allocation failed.

#include "polygon.h"
#include "bitmap.h"
#include "common.h"
#include <stdio.h>
#include <math.h>
#include <malloc.h>


// Describes the beginning and ending X coordinates of a single
// horizontal line
struct HLineData 
{
    int startX; // X coordinate of leftmost pixel in line 
    int endX;   // X coordinate of rightmost pixel in line 
};


// Describes a Length-long series of horizontal lines, all assumed to
// be on contiguous scan lines starting at YStart and proceeding
// downward (used to describe a scan-converted polygon to the
// low-level hardware-dependent drawing code)
struct HLineList 
{
    int numLines;
    int startY;               // Y coordinate of topmost line 
    HLineData *hline;         // pointer to list of horz lines 
};


// Advances the index by one vertex forward through the vertex list,
// wrapping at the end of the list
#define INDEX_FORWARD(index) index = (index + 1) % vertexList->numPoints;

// Advances the index by one vertex backward through the vertex list,
// wrapping at the start of the list
#define INDEX_BACKWARD(index) index = (index - 1 + vertexList->numPoints) % vertexList->numPoints;

// Advances the index by one vertex either forward or backward through
// the vertex list, wrapping at either end of the list
#define INDEX_MOVE(index, direction)              \
    if (direction > 0) { INDEX_FORWARD(index); }  \
    else               { INDEX_BACKWARD(index); }


// Scan converts an edge from (X1,Y1) to (X2,Y2), not including the
// point at (X2,Y2). If SkipFirst == 1, the point at (X1,Y1) isn't
// drawn; if SkipFirst == 0, it is. For each scan line, the pixel
// closest to the scanned edge without being to the left of the
// scanned edge is chosen. Uses an all-integer approach for speed and
// precision.
static void ScanEdge(int x1, int y1, int x2, int y2, int setXStart,
                     int skipFirst, HLineData **edgePointPtr)
{
    int errorTerm, errorTermAdvance, xMajorAdvanceAmt;

    HLineData *workingEdgePointPtr = *edgePointPtr; // avoid double dereference 
    int deltaX = x2 - x1;
    int advanceAmt = (deltaX > 0) ? 1 : -1;
    // direction in which X moves (Y2 is always > Y1, so Y always counts up)

    int height = y2 - y1;
    if (height <= 0)  // Y length of the edge 
        return;     // guard against 0-length and horizontal edges 

    // Figure out whether the edge is vertical, diagonal, X-major
    // (mostly horizontal), or Y-major (mostly vertical) and handle
    // appropriately
    int width = abs(deltaX);
    if (width == 0) 
    {
        // The edge is vertical; special-case by just storing the same
        // X coordinate for every scan line
        // Scan the edge for each scan line in turn 
        for (int i = height - skipFirst; i-- > 0; workingEdgePointPtr++) 
        {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;
        }
    } 
    else if (width == height) 
    {
        // The edge is diagonal; special-case by advancing the X
        // coordinate 1 pixel for each scan line
        if (skipFirst) // skip the first point if so indicated 
            x1 += advanceAmt; // move 1 pixel to the left or right 

        // Scan the edge for each scan line in turn 
        for (int i = height - skipFirst; i-- > 0; workingEdgePointPtr++) 
        {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;
            x1 += advanceAmt; // move 1 pixel to the left or right 
        }
    } 
    else if (height > width) 
    {
        // Edge is closer to vertical than horizontal (Y-major) 
        if (deltaX >= 0)
            errorTerm = 0; // initial error term going left->right 
        else
            errorTerm = -height + 1;   // going right->left 
        
        if (skipFirst)    // skip the first point if so indicated 
        {
            // Determine whether it's time for the X coord to advance 
            if ((errorTerm += width) > 0) 
            {
                x1 += advanceAmt; // move 1 pixel to the left or right 
                errorTerm -= height; // advance ErrorTerm to next point 
            }
        }

        // Scan the edge for each scan line in turn 
        for (int i = height - skipFirst; i-- > 0; workingEdgePointPtr++) 
        {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;

            // Determine whether it's time for the X coord to advance 
            if ((errorTerm += width) > 0) 
            {
                x1 += advanceAmt; // move 1 pixel to the left or right 
                errorTerm -= height; // advance ErrorTerm to correspond 
            }
        }
    } 
    else 
    {
        // Edge is closer to horizontal than vertical (X-major) 
        // Minimum distance to advance X each time 
        xMajorAdvanceAmt = (width / height) * advanceAmt;

        // Error term advance for deciding when to advance X 1 extra 
        errorTermAdvance = width % height;
        if (deltaX >= 0)
            errorTerm = 0; // initial error term going left->right 
        else
            errorTerm = -height + 1;   // going right->left 

        if (skipFirst)    // skip the first point if so indicated 
        {
            x1 += xMajorAdvanceAmt;    // move X minimum distance 
            // Determine whether it's time for X to advance one extra 
            if ((errorTerm += errorTermAdvance) > 0) 
            {
                x1 += advanceAmt;       // move X one more 
                errorTerm -= height; // advance ErrorTerm to correspond 
            }
        }
        // Scan the edge for each scan line in turn 
        for (int i = height - skipFirst; i-- > 0; workingEdgePointPtr++) 
        {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;

            x1 += xMajorAdvanceAmt;    // move X minimum distance 
            
            // Determine whether it's time for X to advance one extra 
            if ((errorTerm += errorTermAdvance) > 0) 
            {
                x1 += advanceAmt;       // move X one more 
                errorTerm -= height; // advance ErrorTerm to correspond 
            }
        }
    }

    *edgePointPtr = workingEdgePointPtr;   // advance caller's ptr 
}


static void DrawHorizontalLineList(BitmapRGBA *bmp, HLineList *hLines, RGBAColour col)
{
    int startY = hLines->startY;
    int len = hLines->numLines;
    int endY = startY + len;

    // Clip against the top of the bitmap
    if (startY < 0)
    {
        if (endY < 0)
            return;

        hLines -= startY;
        len += startY;
        startY = 0;
    }

    // Clip against the bottom of the bitmap
    if (endY >= (int)bmp->height)
    {
        if (startY >= (int)bmp->height)
            return;

        len = bmp->height - startY;
    }

    // Draw the hlines
    HLineData *firstLine = hLines->hline;
    HLineData *lastLine = firstLine + len;
    if (col.a != 255)
    {
        // Alpha blended path...
        int y = startY;
        for (HLineData * __restrict line = firstLine; line < lastLine; line++, y++)
            HLine(bmp, line->startX, y, line->endX - line->startX, col);
    }
    else
    {
        // Solid colour path...
        RGBAColour * __restrict row = bmp->pixels + bmp->width * hLines->startY;
        for (HLineData * __restrict line = firstLine; line < lastLine; line++) 
        {
            // Clip against sides of bitmap
            int startX = IntMax(0, line->startX);
            int endX = IntMin(bmp->width, line->endX);
            if (startX >= (int)bmp->width || endX < 0)
                continue;

            for (int x = startX; x < endX; x++)
                row[x] = col;
            row += bmp->width;
        }
    }
}


int FillConvexPolygon(BitmapRGBA *bmp, PointListHeader *vertexList, RGBAColour col,
                      int xOffset, int yOffset)
{
    // Point to the vertex list 
    Point *vertices = vertexList->points;

    // Scan the list to find the top and bottom of the polygon 
    if (vertexList->numPoints == 0)
        return 1;  // reject null polygons 

    int minIndexL, maxIndex, minPointY;
    int maxPointY = minPointY = vertices[minIndexL = maxIndex = 0].y;
    for (int i = 1; i < vertexList->numPoints; i++) 
    {
        if (vertices[i].y < minPointY)
            minPointY = vertices[minIndexL = i].y; // new top 
        else if (vertices[i].y > maxPointY)
            maxPointY = vertices[maxIndex = i].y; // new bottom 
    }

    if (minPointY == maxPointY)
        return 1;  // polygon is 0-height; avoid infinite loop below 

    // Scan in ascending order to find the last top-edge point 
    int minIndexR = minIndexL;
    while (vertices[minIndexR].y == minPointY)
        INDEX_FORWARD(minIndexR);
    INDEX_BACKWARD(minIndexR); // back up to last top-edge point 

    // Now scan in descending order to find the first top-edge point 
    while (vertices[minIndexL].y == minPointY)
        INDEX_BACKWARD(minIndexL);
    INDEX_FORWARD(minIndexL); // back up to first top-edge point 

    // Figure out which direction through the vertex list from the top
    // vertex is the left edge and which is the right
    int leftEdgeDir = -1; // assume left edge runs down thru vertex list 
    int topIsFlat = (vertices[minIndexL].x != vertices[minIndexR].x) ? 1 : 0;
    if (topIsFlat == 1) 
    {
        // If the top is flat, just see which of the ends is leftmost 
        if (vertices[minIndexL].x > vertices[minIndexR].x) 
        {
            leftEdgeDir = 1;        // left edge runs up through vertex list 
            int temp = minIndexL;   // swap the indices so MinIndexL   
            minIndexL = minIndexR;  // points to the start of the left 
            minIndexR = temp;       // edge, similarly for MinIndexR   
        }
    } 
    else 
    {
        // Point to the downward end of the first line of each of the
        // two edges down from the top
        int nextIndex = minIndexR;
        INDEX_FORWARD(nextIndex);
        int previousIndex = minIndexL;
        INDEX_BACKWARD(previousIndex);
        // Calculate X and Y lengths from the top vertex to the end of
        // the first line down each edge; use those to compare slopes
        // and see which line is leftmost
        int deltaXN = vertices[nextIndex].x - vertices[minIndexL].x;
        int deltaYN = vertices[nextIndex].y - vertices[minIndexL].y;
        int deltaXP = vertices[previousIndex].x - vertices[minIndexL].x;
        int deltaYP = vertices[previousIndex].y - vertices[minIndexL].y;
        if ((deltaXN * deltaYP - deltaYN * deltaXP) < 0L) 
        {
            leftEdgeDir = 1;        // left edge runs up through vertex list 
            int temp = minIndexL;   // swap the indices so MinIndexL   
            minIndexL = minIndexR;  // points to the start of the left 
            minIndexR = temp;       // edge, similarly for MinIndexR   
        }
    }

    // Set the # of scan lines in the polygon, skipping the bottom edge
    // and also skipping the top vertex if the top isn't flat because
    // in that case the top vertex has a right edge component, and set
    // the top scan line to draw, which is likewise the second line of
    // the polygon unless the top is flat
    HLineList workingHLineList;
    workingHLineList.numLines = maxPointY - minPointY - 1 + topIsFlat;
    if (workingHLineList.numLines <= 0)
        return 1;  // there's nothing to draw, so we're done 

    workingHLineList.startY = yOffset + minPointY + 1 - topIsFlat;

    // Get memory in which to store the line list we generate 
    workingHLineList.hline = (HLineData *)alloca(sizeof(HLineData) * workingHLineList.numLines);
    if (workingHLineList.hline == NULL)
        return 0;  // couldn't get memory for the line list 

    // Scan the left edge and store the boundary points in the list 
    // Initial pointer for storing scan converted left-edge coords 
    HLineData *edgePoint = workingHLineList.hline;
    
    // Start from the top of the left edge 
    int previousIndex = minIndexL;
    int currentIndex = previousIndex;
    
    // Skip the first point of the first line unless the top is flat;
    // if the top isn't flat, the top vertex is exactly on a right
    // edge and isn't drawn
    int skipFirst = topIsFlat ? 0 : 1;
    
    // Scan convert each line in the left edge from top to bottom 
    do {
        INDEX_MOVE(currentIndex, leftEdgeDir);
        ScanEdge(vertices[previousIndex].x + xOffset,
            vertices[previousIndex].y,
            vertices[currentIndex].x + xOffset,
            vertices[currentIndex].y, 1, skipFirst, &edgePoint);
        previousIndex = currentIndex;
        skipFirst = 0; // scan convert the first point from now on 
    } while (currentIndex != maxIndex);

    // Scan the right edge and store the boundary points in the list 
    edgePoint = workingHLineList.hline;
    previousIndex = currentIndex = minIndexR;
    skipFirst = topIsFlat ? 0 : 1;

    // Scan convert the right edge, top to bottom. X coordinates are
    // adjusted 1 to the left, effectively causing scan conversion of
    // the nearest points to the left of but not exactly on the edge
    do {
        INDEX_MOVE(currentIndex, -leftEdgeDir);
        ScanEdge(vertices[previousIndex].x + xOffset - 1,
            vertices[previousIndex].y,
            vertices[currentIndex].x + xOffset - 1,
            vertices[currentIndex].y, 0, skipFirst, &edgePoint);
        previousIndex = currentIndex;
        skipFirst = 0; // scan convert the first point from now on 
    } while (currentIndex != maxIndex);

    // Draw the line list representing the scan converted polygon 
    DrawHorizontalLineList(bmp, &workingHLineList, col);

    return 1;
}

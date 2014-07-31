/* Color-fills a convex polygon. All vertices are offset by (XOffset,
YOffset). "Convex" means that every horizontal line drawn through
the polygon at any point would cross exactly two active edges
(neither horizontal lines nor zero-length edges count as active
edges; both are acceptable anywhere in the polygon), and that the
right & left edges never cross. (It's OK for them to touch, though,
so long as the right edge never crosses over to the left of the
left edge.) Nonconvex polygons won't be drawn properly. Returns 1
for success, 0 if memory allocation failed.
*/

#include "polygon.h"
#include "bitmap.h"
#include "common.h"
#include <stdio.h>
#include <math.h>
#include <malloc.h>


/* Describes the beginning and ending X coordinates of a single
horizontal line */
struct HLineData {
    int startX; // X coordinate of leftmost pixel in line 
    int endX;   // X coordinate of rightmost pixel in line 
};

/* Describes a Length-long series of horizontal lines, all assumed to
be on contiguous scan lines starting at YStart and proceeding
downward (used to describe a scan-converted polygon to the
low-level hardware-dependent drawing code) */
struct HLineList {
    int numLines;
    int startY;               // Y coordinate of topmost line 
    HLineData *hline;         // pointer to list of horz lines 
};


/* Advances the index by one vertex forward through the vertex list,
wrapping at the end of the list */
#define INDEX_FORWARD(Index) Index = (Index + 1) % vertexList->Length;

/* Advances the index by one vertex backward through the vertex list,
wrapping at the start of the list */
#define INDEX_BACKWARD(Index) Index = (Index - 1 + vertexList->Length) % vertexList->Length;

/* Advances the index by one vertex either forward or backward through
the vertex list, wrapping at either end of the list */
#define INDEX_MOVE(Index, Direction)                                  \
    if (Direction > 0)                                                \
        Index = (Index + 1) % vertexList->Length;                     \
    else                                                              \
        Index = (Index - 1 + vertexList->Length) % vertexList->Length;


/* Scan converts an edge from (X1,Y1) to (X2,Y2), not including the
point at (X2,Y2). If SkipFirst == 1, the point at (X1,Y1) isn't
drawn; if SkipFirst == 0, it is. For each scan line, the pixel
closest to the scanned edge without being to the left of the
scanned edge is chosen. Uses an all-integer approach for speed and
precision.
*/
void ScanEdge(int x1, int y1, int x2, int y2, int setXStart,
              int skipFirst, HLineData **edgePointPtr)
{
    int deltaX, height, width, errorTerm, i;
    int errorTermAdvance, xMajorAdvanceAmt;

    HLineData *workingEdgePointPtr = *edgePointPtr; // avoid double dereference 
    int advanceAmt = ((deltaX = x2 - x1) > 0) ? 1 : -1;
    // direction in which X moves (Y2 is always > Y1, so Y always counts up)

    if ((height = y2 - y1) <= 0)  // Y length of the edge 
        return;     // guard against 0-length and horizontal edges 

    /* Figure out whether the edge is vertical, diagonal, X-major
    (mostly horizontal), or Y-major (mostly vertical) and handle
    appropriately */
    if ((width = abs(deltaX)) == 0) {
        /* The edge is vertical; special-case by just storing the same
        X coordinate for every scan line */
        // Scan the edge for each scan line in turn 
        for (i = height - skipFirst; i-- > 0; workingEdgePointPtr++) {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;
        }
    } else if (width == height) {
        /* The edge is diagonal; special-case by advancing the X
        coordinate 1 pixel for each scan line */
        if (skipFirst) // skip the first point if so indicated 
            x1 += advanceAmt; // move 1 pixel to the left or right 
        // Scan the edge for each scan line in turn 
        for (i = height - skipFirst; i-- > 0; workingEdgePointPtr++) {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;
            x1 += advanceAmt; // move 1 pixel to the left or right 
        }
    } else if (height > width) {
        // Edge is closer to vertical than horizontal (Y-major) 
        if (deltaX >= 0)
            errorTerm = 0; // initial error term going left->right 
        else
            errorTerm = -height + 1;   // going right->left 
        if (skipFirst) {   // skip the first point if so indicated 
            // Determine whether it's time for the X coord to advance 
            if ((errorTerm += width) > 0) {
                x1 += advanceAmt; // move 1 pixel to the left or right 
                errorTerm -= height; // advance ErrorTerm to next point 
            }
        }
        // Scan the edge for each scan line in turn 
        for (i = height - skipFirst; i-- > 0; workingEdgePointPtr++) {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;
            // Determine whether it's time for the X coord to advance 
            if ((errorTerm += width) > 0) {
                x1 += advanceAmt; // move 1 pixel to the left or right 
                errorTerm -= height; // advance ErrorTerm to correspond 
            }
        }
    } else {
        // Edge is closer to horizontal than vertical (X-major) 
        // Minimum distance to advance X each time 
        xMajorAdvanceAmt = (width / height) * advanceAmt;
        // Error term advance for deciding when to advance X 1 extra 
        errorTermAdvance = width % height;
        if (deltaX >= 0)
            errorTerm = 0; // initial error term going left->right 
        else
            errorTerm = -height + 1;   // going right->left 
        if (skipFirst) {   // skip the first point if so indicated 
            x1 += xMajorAdvanceAmt;    // move X minimum distance 
            // Determine whether it's time for X to advance one extra 
            if ((errorTerm += errorTermAdvance) > 0) {
                x1 += advanceAmt;       // move X one more 
                errorTerm -= height; // advance ErrorTerm to correspond 
            }
        }
        // Scan the edge for each scan line in turn 
        for (i = height - skipFirst; i-- > 0; workingEdgePointPtr++) {
            // Store the X coordinate in the appropriate edge list 
            if (setXStart == 1)
                workingEdgePointPtr->startX = x1;
            else
                workingEdgePointPtr->endX = x1;
            x1 += xMajorAdvanceAmt;    // move X minimum distance 
            // Determine whether it's time for X to advance one extra 
            if ((errorTerm += errorTermAdvance) > 0) {
                x1 += advanceAmt;       // move X one more 
                errorTerm -= height; // advance ErrorTerm to correspond 
            }
        }
    }

    *edgePointPtr = workingEdgePointPtr;   // advance caller's ptr 
}


void DrawHorizontalLineList(BitmapRGBA *bmp, HLineList *hLines, RGBAColour col)
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
    int i, MinIndexL, MaxIndex, MinIndexR, SkipFirst, Temp;
    int MinPoint_Y, MaxPoint_Y, TopIsFlat, LeftEdgeDir;
    int NextIndex, CurrentIndex, PreviousIndex;
    int DeltaXN, DeltaYN, DeltaXP, DeltaYP;
    HLineList WorkingHLineList;
    HLineData *EdgePointPtr;
    Point *VertexPtr;

    // Point to the vertex list 
    VertexPtr = vertexList->PointPtr;

    // Scan the list to find the top and bottom of the polygon 
    if (vertexList->Length == 0)
        return(1);  // reject null polygons 
    MaxPoint_Y = MinPoint_Y = VertexPtr[MinIndexL = MaxIndex = 0].Y;
    for (i = 1; i < vertexList->Length; i++) {
        if (VertexPtr[i].Y < MinPoint_Y)
            MinPoint_Y = VertexPtr[MinIndexL = i].Y; // new top 
        else if (VertexPtr[i].Y > MaxPoint_Y)
            MaxPoint_Y = VertexPtr[MaxIndex = i].Y; // new bottom 
    }
    if (MinPoint_Y == MaxPoint_Y)
        return(1);  // polygon is 0-height; avoid infinite loop below 

    // Scan in ascending order to find the last top-edge point 
    MinIndexR = MinIndexL;
    while (VertexPtr[MinIndexR].Y == MinPoint_Y)
        INDEX_FORWARD(MinIndexR);
    INDEX_BACKWARD(MinIndexR); // back up to last top-edge point 

    // Now scan in descending order to find the first top-edge point 
    while (VertexPtr[MinIndexL].Y == MinPoint_Y)
        INDEX_BACKWARD(MinIndexL);
    INDEX_FORWARD(MinIndexL); // back up to first top-edge point 

    // Figure out which direction through the vertex list from the top
    // vertex is the left edge and which is the right
    LeftEdgeDir = -1; // assume left edge runs down thru vertex list 
    if ((TopIsFlat = (VertexPtr[MinIndexL].X !=
        VertexPtr[MinIndexR].X) ? 1 : 0) == 1) {
            // If the top is flat, just see which of the ends is leftmost 
            if (VertexPtr[MinIndexL].X > VertexPtr[MinIndexR].X) {
                LeftEdgeDir = 1;  // left edge runs up through vertex list 
                Temp = MinIndexL;       // swap the indices so MinIndexL   
                MinIndexL = MinIndexR;  // points to the start of the left 
                MinIndexR = Temp;       // edge, similarly for MinIndexR   
            }
    } else {
        /* Point to the downward end of the first line of each of the
        two edges down from the top */
        NextIndex = MinIndexR;
        INDEX_FORWARD(NextIndex);
        PreviousIndex = MinIndexL;
        INDEX_BACKWARD(PreviousIndex);
        /* Calculate X and Y lengths from the top vertex to the end of
        the first line down each edge; use those to compare slopes
        and see which line is leftmost */
        DeltaXN = VertexPtr[NextIndex].X - VertexPtr[MinIndexL].X;
        DeltaYN = VertexPtr[NextIndex].Y - VertexPtr[MinIndexL].Y;
        DeltaXP = VertexPtr[PreviousIndex].X - VertexPtr[MinIndexL].X;
        DeltaYP = VertexPtr[PreviousIndex].Y - VertexPtr[MinIndexL].Y;
        if (((long)DeltaXN * DeltaYP - (long)DeltaYN * DeltaXP) < 0L) {
            LeftEdgeDir = 1;  // left edge runs up through vertex list 
            Temp = MinIndexL;       // swap the indices so MinIndexL   
            MinIndexL = MinIndexR;  // points to the start of the left 
            MinIndexR = Temp;       // edge, similarly for MinIndexR   
        }
    }

    /* Set the # of scan lines in the polygon, skipping the bottom edge
    and also skipping the top vertex if the top isn't flat because
    in that case the top vertex has a right edge component, and set
    the top scan line to draw, which is likewise the second line of
    the polygon unless the top is flat */
    if ((WorkingHLineList.numLines =
        MaxPoint_Y - MinPoint_Y - 1 + TopIsFlat) <= 0)
        return(1);  // there's nothing to draw, so we're done 
    WorkingHLineList.startY = yOffset + MinPoint_Y + 1 - TopIsFlat;

    // Get memory in which to store the line list we generate 
    //   WorkingHLineList.HLinePtr = (HLineData *)malloc(sizeof(HLineData) * WorkingHLineList.Length);
    WorkingHLineList.hline = (HLineData *)alloca(sizeof(HLineData) * WorkingHLineList.numLines);
    if (WorkingHLineList.hline == NULL)
        return(0);  // couldn't get memory for the line list 

    // Scan the left edge and store the boundary points in the list 
    // Initial pointer for storing scan converted left-edge coords 
    EdgePointPtr = WorkingHLineList.hline;
    // Start from the top of the left edge 
    PreviousIndex = CurrentIndex = MinIndexL;
    /* Skip the first point of the first line unless the top is flat;
    if the top isn't flat, the top vertex is exactly on a right
    edge and isn't drawn */
    SkipFirst = TopIsFlat ? 0 : 1;
    // Scan convert each line in the left edge from top to bottom 
    do {
        INDEX_MOVE(CurrentIndex,LeftEdgeDir);
        ScanEdge(VertexPtr[PreviousIndex].X + xOffset,
            VertexPtr[PreviousIndex].Y,
            VertexPtr[CurrentIndex].X + xOffset,
            VertexPtr[CurrentIndex].Y, 1, SkipFirst, &EdgePointPtr);
        PreviousIndex = CurrentIndex;
        SkipFirst = 0; // scan convert the first point from now on 
    } while (CurrentIndex != MaxIndex);

    // Scan the right edge and store the boundary points in the list 
    EdgePointPtr = WorkingHLineList.hline;
    PreviousIndex = CurrentIndex = MinIndexR;
    SkipFirst = TopIsFlat ? 0 : 1;
    /* Scan convert the right edge, top to bottom. X coordinates are
    adjusted 1 to the left, effectively causing scan conversion of
    the nearest points to the left of but not exactly on the edge */
    do {
        INDEX_MOVE(CurrentIndex,-LeftEdgeDir);
        ScanEdge(VertexPtr[PreviousIndex].X + xOffset - 1,
            VertexPtr[PreviousIndex].Y,
            VertexPtr[CurrentIndex].X + xOffset - 1,
            VertexPtr[CurrentIndex].Y, 0, SkipFirst, &EdgePointPtr);
        PreviousIndex = CurrentIndex;
        SkipFirst = 0; // scan convert the first point from now on 
    } while (CurrentIndex != MaxIndex);

    // Draw the line list representing the scan converted polygon 
    DrawHorizontalLineList(bmp, &WorkingHLineList, col);

    // Release the line list's memory and we're successfully done 
    //   free(WorkingHLineList.HLinePtr);
    return(1);
}

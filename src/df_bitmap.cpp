#include "df_bitmap.h"

#include "df_colour.h"
#include "df_common.h"

#include <algorithm>
#include <math.h>
#include <memory.h>
#include <stdlib.h>


DfBitmap *DfCreateBitmap(unsigned width, unsigned height)
{
	DfBitmap *bmp = new DfBitmap;
	bmp->width = width;
	bmp->height = height;
	bmp->pixels = new DfColour[width * height];
	bmp->lines = new DfColour *[height];

	DebugAssert(bmp->pixels && bmp->lines);

	for (unsigned y = 0; y < height; ++y)
		bmp->lines[y] = &bmp->pixels[y * width];

    return bmp;
}


void DfDeleteBitmap(DfBitmap *bmp)
{
	bmp->width = -1;
	bmp->height = -1;
	delete [] bmp->pixels;
	delete [] bmp->lines;
	bmp->pixels = NULL;
	bmp->lines = NULL;
}


#define GetLine(bmp, y) ((bmp)->pixels + (bmp)->width * (y))


void ClearBitmap(DfBitmap *bmp, DfColour colour)
{
    RectFill(bmp, 0, 0, bmp->width, bmp->height, colour);
}


DfColour GetPixUnclipped(DfBitmap *bmp, unsigned x, unsigned y)
{
	return GetLine(bmp, y)[x];
}


void PutPix(DfBitmap *bmp, unsigned x, unsigned y, DfColour colour)
{
	if (x >= bmp->width || y >= bmp->height)
		return;

	PutPixUnclipped(bmp, x, y, colour);
}


DfColour GetPix(DfBitmap *bmp, unsigned x, unsigned y)
{
	if (x >= bmp->width || y >= bmp->height)
		return g_colourBlack;

	return GetLine(bmp, y)[x]; 
}


void HLineUnclipped(DfBitmap *bmp, int x, int y, unsigned len, DfColour c)
{
    DfColour * __restrict row = GetLine(bmp, y) + x;
    if (c.a == 255)
    {
        //         for (unsigned i = 0; i < len; i++)
        //             row[i] = c;
        __stosd((unsigned long*)row, c.c, len);
    }
    else
    {
        unsigned crb = (c.c & 0xff00ff) * c.a;
        unsigned cg = c.g * c.a;

        unsigned char invA = 255 - c.a;
        for (unsigned i = 0; i < len; i++)
        {
            DfColour *pixel = row + i;
            unsigned rb = (pixel->c & 0xff00ff) * invA + crb;
            unsigned g = pixel->g * invA + cg;

            pixel->c = rb >> 8;
            pixel->g = g >> 8;
        }
    }
}


void HLine(DfBitmap *bmp, int x, int y, unsigned len, DfColour c)
{
    if (len > (1<<31))
        return;

    // Clip against top and bottom of bmp
    if (unsigned(y) >= bmp->height)
        return;

    if (len == 0)
        return;

    // Clip against left
    if (x < 0)
    {
        if ((int)len <= -x)
            return;
        len += x;
        x = 0;
    }

    // Clip against right
    if (x + (int)len > int(bmp->width))
    {
        if (x > (int)bmp->width)
            return;

        len = bmp->width - x;
    }

    HLineUnclipped(bmp, x, y, len, c);
}


void VLineUnclipped(DfBitmap *bmp, int x, int y, unsigned len, DfColour c)
{
    DfColour * __restrict pixel = GetLine(bmp, y) + x;
    unsigned const bw = bmp->width;
    if (c.a == 255)
    {
        for (unsigned i = 0; i < len; i++)
        {
            *pixel = c;
            pixel += bw;
        }
    }
    else
    {
        unsigned char invA = 255 - c.a;
        unsigned cr = c.r * c.a;
        unsigned cg = c.g * c.a;
        unsigned cb = c.b * c.a;

        for (unsigned i = 0; i < len; i++)
        {
            pixel->r = (pixel->r * invA + cr) >> 8;
            pixel->g = (pixel->g * invA + cg) >> 8;
            pixel->b = (pixel->b * invA + cb) >> 8;
            pixel += bw;
        }
    }
}


void VLine(DfBitmap *bmp, int x, int y, unsigned len, DfColour c)
{
    if (len == 0 || len > (1 << 31))
        return;

    // Clip against left and right of bmp
    if (unsigned(x) > bmp->width)
        return;

    // Clip against top
    if (y < 0)
    {
        if ((int)len <= -y)
            return;
        len += y;
        y = 0;
    }

    // Clip against bottom
    if (y > (int)bmp->height)
        return;
    if (y + (int)len > int(bmp->height))
        len = bmp->height - y;

    VLineUnclipped(bmp, x, y, len, c);
}


void DrawLine(DfBitmap *bmp, int x1, int y1, int x2, int y2, DfColour colour)
{
    // This implementation is based on that presented in Michael Abrash's Zen of
    // Graphics Programming (2nd Edition), listing 15-1, page 250.

    // We'll always draw top to bottom, to reduce the number of cases we have to
    // handle, and to make lines between the same endpoints draw the same pixels
    if (y1 > y2)
    {
        int temp = y1;
        y1 = y2;
        y2 = temp;
        temp = x1;
        x1 = x2;
        x2 = temp;
    }

    int xDelta = x2 - x1;
    int yDelta = y2 - y1;
    
    // Special case horizontal and vertical lines
    if (xDelta == 0)
        return VLine(bmp, x1, y1, yDelta+1, colour);
    if (yDelta == 0)
    {
        if (xDelta > 0)
            return HLine(bmp, x1, y1, xDelta+1, colour);
        else
            return HLine(bmp, x2, y1, 1-xDelta, colour);
    }

    // Get a constant copy of the bitmap width so the compiler can assume it
    // doesn't get changed by anything (like an aliased pointer).
    unsigned bmpWidth = bmp->width;

    // Clip first end of line
    if ((unsigned)x1 >= bmpWidth || (unsigned)y1 >= bmp->height ||
        (unsigned)x2 >= bmpWidth || (unsigned)y2 >= bmp->height)
    {
        // Parametric line equation can be written as:
        //   x = rx + t * ux
        //   y = ry + t * uy
        // Where (x,y) is a point on the line. (rx,ry) is another 
        // point on the line. (ux, uy) is a vector along the line.
        // We can use (xDelta, yDelta) for (ux, uy).
        // t is a real number that ranges from 0 to 1 for our line segment.
        //  - t = 0 gives us (x1, y1)
        //  - t = 1 gives us (x2, y2)
        // Now clipping our line against the bitmap edges can be done by
        // seeing if any of the edges limits 't' to a range smaller than
        // 0 to 1.

        double tMin = 0.0;
        double tMax = 1.0;
        double t;

        // Find t when the line intersects the top of the bitmap.
        // If y = 0 then 0 = ry + t * yDelta
        // or t = -ry / yDelta
        t = -double(y1) / double(yDelta);
        if (t > 1.0)
            return;
        if (t > tMin)
            tMin = t;

        // Find t when the line intersects the bottom of the bitmap.
        // If y = height then height = ry + t * yDelta
        // or t = (height - ry) / yDelta;
        t = double(int(bmp->height) - 1 - y1) / double(yDelta);
        if (t < 0.0)
            return;
        if (t < tMax)
            tMax = t;

        // Find t when the line intersects the left edge of the bitmap.
        // If x = 0 then 0 = rx + t * xDelta
        // Solving for t gives -rx = t * xDelta, or t = -rx / xDelta
        t = -double(x1) / double(xDelta);
        if (xDelta > 0)
        {
            if (t > tMin)
                tMin = t;
        }
        else
        {
            if (t < tMax)
                tMax = t;
        }

        // Find t when the line intersects the right edge of the bitmap.
        // If x = width then width = rx + t * xDelta.
        // Solving for t gives width - rx = t * xDelta
        // or t = (width - rx) / xDelta
        t = double(int(bmpWidth) - 1 - x1) / double(xDelta);
        if (xDelta > 0)
        {
            if (t < tMax)
                tMax = t;
        }
        else
        {
            if (t > tMin)
                tMin = t;
        }

        if (tMin > tMax)
            return;

        // Now recalc (x1,y1) and (x2,y2)
        int nx1 = x1 + RoundToInt(xDelta * tMin);
        int ny1 = y1 + RoundToInt(yDelta * tMin);
        int nx2 = x1 + RoundToInt(xDelta * tMax);
        int ny2 = y1 + RoundToInt(yDelta * tMax);

        DebugAssert(nx1 >= 0);
        DebugAssert(nx1 < (int)bmpWidth);
        DebugAssert(ny1 >= 0);
        DebugAssert(ny1 < (int)bmp->height);
        DebugAssert(nx2 >= 0);
        DebugAssert(nx2 < (int)bmpWidth);
        DebugAssert(ny1 >= 0);
        DebugAssert(ny1 < (int)bmp->height);

        x1 = nx1;
        x2 = nx2;
        y1 = ny1;
        y2 = ny2;

        xDelta = x2 - x1;
        yDelta = y2 - y1;

        // Special case horizontal and vertical lines
        if (xDelta == 0)
            return VLineUnclipped(bmp, x1, y1, yDelta, colour);
        if (yDelta == 0)
        {
            if (xDelta > 0)
                return HLineUnclipped(bmp, x1, y1, xDelta, colour);
            else
                return HLineUnclipped(bmp, x2, y1, -xDelta, colour);
        }
    }

    int xAdvance = 1;
    if (xDelta < 0)
    {
        xAdvance = -1;
        xDelta = -xDelta;
    }

    // Special case diagonal line
    if (xDelta == yDelta)
    {
        for (int i = 0; i <= xDelta; i++)
            PutPixUnclipped(bmp, x1 + i * xAdvance, y1 + i, colour);
        return;
    }

    DfColour * __restrict pixel = GetLine(bmp, y1) + x1;

    // Is the line X major or y major?
    if (xDelta >= yDelta)
    {
        // Min number pixels in a run in this line
        int wholeStep = xDelta / yDelta;
        
        // Error term adjust each time Y steps by 1; used to tell when one
        // extra pixel should be drawn as part of a run, to account for
        // fractional steps along the X axis per 1-pixel steps along Y.
        int adjUp = (xDelta %  yDelta) * 2;
        
        // Error term adjust when the error term turns over, used to factor
        // out the X step made at that time.
        int adjDown = yDelta * 2;
        
        // Initial error term; reflects an initial step of 0.5 along the Y axis
        int errorTerm = (xDelta % yDelta) - (yDelta * 2);
        
        // The initial and last runs are partial because Y advances only 0.5
        // for these runs. Divide one full run, plus the initial pixel, between 
        // the initial and last runs.
        int initialPixelCount = (wholeStep / 2) + 1;
        int finalPixelCount = initialPixelCount;

        // If the basic run length is even and there's no fraction
        // advance, we have one pixel that could go to either the initial
        // of last partial run, which we'll arbitrarily allocate to the
        // last run.
        if (adjUp == 0 && (wholeStep & 1) == 0)
            initialPixelCount--;

        // If there're an odd number of pixels per run, we have 1 pixel that can't
        // be allocated to either the initial or last partial run, so we'll add 0.5
        // to the error term so this pixel will be handled by the normal full-run 
        // loop.
        if ((wholeStep & 1) != 0)
            errorTerm += yDelta;

        // Draw the first partial run of pixels
        for (; initialPixelCount; initialPixelCount--)
        {
            *pixel = colour;
            pixel += xAdvance;
        }
        pixel += bmpWidth;

        // Do the main loop
        for (int i = 0; i < yDelta - 1; i++)
        {
            int runLength = wholeStep;
            if ((errorTerm += adjUp) > 0)
            {
                runLength++;
                errorTerm -= adjDown;
            }
        
            for (; runLength; runLength--)
            {
                *pixel = colour;
                pixel += xAdvance;
            }
            pixel += bmpWidth;
        }

        // Draw the last partial run
        for (; finalPixelCount; finalPixelCount--)
        {
            *pixel = colour;
            pixel += xAdvance;
        }
    }
    else
    {
        // Line is Y major

        int wholeStep = yDelta / xDelta;
        int adjUp = (yDelta % xDelta) * 2;
        int adjDown = xDelta * 2;
        int errorTerm = (yDelta % xDelta) - xDelta * 2;
        int initialPixelCount = (wholeStep / 2) + 1;
        int finalPixelCount = initialPixelCount;
        
        if (adjUp == 0 && (wholeStep & 1) == 0)
            initialPixelCount--;

        if ((wholeStep & 1) != 0)
            errorTerm += xDelta;

        // Draw vertical run
        int lineInc = bmpWidth;
        for (; initialPixelCount; initialPixelCount--)
        {
            *pixel = colour;
            pixel += lineInc;
        }
        pixel += xAdvance;

        // Do the main loop
        for (int i = 0; i < xDelta - 1; i++)
        {
            int runlength = wholeStep;
            if ((errorTerm += adjUp) > 0)
            {
                runlength++;
                errorTerm -= adjDown;
            }

            for (; runlength; runlength--)
            {
                *pixel = colour;
                pixel += lineInc;
            }
            pixel += xAdvance;
        }

        // Draw the last partial run
        for (; finalPixelCount; finalPixelCount--)
        {
            *pixel = colour;
            pixel += lineInc;
        }
    }
}


// Calculate the position on a Bezier curve. t is in range 0 to 63356.
void GetBezierPos(int const *a, int const *b, int const *c, int const *d, int t, int *result)
{
    double dt = (double)t / 65536.0;
    double nt = 1.0 - dt;

    double scalar1 = nt*nt*nt;
    double scalar2 = 3*nt*nt*dt;
    double scalar3 = 3*nt*dt*dt;
    double scalar4 = dt*dt*dt;

    result[0] = (int)(a[0] * scalar1 + b[0] * scalar2 + c[0] * scalar3 + d[0] * scalar4);
    result[1] = (int)(a[1] * scalar1 + b[1] * scalar2 + c[1] * scalar3 + d[1] * scalar4);
}


void GetBezierDir(int const *a, int const *b, int const *c, int const *d, int t, int *result)
{
    double dt = (double)t / 65536.0;
    double nt = 1.0 - dt;

    double scalar1 = -3.0*nt*nt;
    double scalar2 = 3.0*(1.0 - 4.0*dt + 3.0*dt*dt);
    double scalar3 = 3.0*(2.0*dt - 3.0*dt*dt);
    double scalar4 = 3.0*dt*dt;

    result[0] = (int)(a[0] * scalar1 + b[0] * scalar2 + c[0] * scalar3 + d[0] * scalar4);
    result[1] = (int)(a[1] * scalar1 + b[1] * scalar2 + c[1] * scalar3 + d[1] * scalar4);
}


static double GetLineLen(int const *a, int const *b)
{
    int dx = a[0] - b[0];
    int dy = a[1] - b[1];
    double len = sqrt((double)dx * dx + (double)dy * dy);
    return len;
}


void DrawBezier(DfBitmap *bmp, int const *a, int const *b, int const *c, int const *d, DfColour col)
{
    // Calculate approximate length of a Bezier curve by sampling at a low 
    // resolution and summing distances between samples.
    double totalLen = 0;
    {
        int v[2];
        v[0] = a[0];
        v[1] = a[1];
       
        for (int t = 65536/4; t < 65536; t += 16384)
        {
            int w[2];
            GetBezierPos(a, b, c, d, t, w);
            
            totalLen += GetLineLen(v, w);

            v[0] = w[0];
            v[1] = w[1];
        }

        totalLen += GetLineLen(v, d);
    }

    // Use curve length to calculate a good increment for loop below
    int inc = 1000;
    if (totalLen > 0.0)
        inc = (int)(200000.0 / totalLen);

    // Draw the bezier by sampling at a higher resolution and drawing straight lines between segments
    int oldP[2];
    oldP[0] = a[0];
    oldP[1] = a[1];
    for (int t = inc; t < 65536; t += inc)
    {
        int p[2];
        GetBezierPos(a, b, c, d, t, p);
        DrawLine(bmp, oldP[0], oldP[1], p[0], p[1], col);
        oldP[0] = p[0];
        oldP[1] = p[1];
    }

    DrawLine(bmp, oldP[0], oldP[1], d[0], d[1], col);
}


void RectFill(DfBitmap *bmp, int x, int y, unsigned w, unsigned h, DfColour c)
{
    // Get a constant copy of the bitmap width so the compiler can assume it
    // doesn't get changed by anything (like an aliased pointer).
    unsigned bmpWidth = bmp->width;

    if (w > bmpWidth)
        w = bmpWidth;

    int x1 = IntMax(x, 0);
    int x2 = IntMin(x + w, bmp->width) - 1;
    int y1 = IntMax(y, 0);
    int y2 = IntMin(y + h, bmp->height) - 1;

    if (x1 > (int)bmpWidth || x2 < 0 || y1 > (int)bmp->height || y2 < 0)
        return;

    w = x2 - x1 + 1;
  
    DfColour * __restrict line = GetLine(bmp, y1) + x1;
    if (c.a == 255)
    {
        for (int a = y1; a <= y2; a++)
        {
//             for (int b = x1; b <= x2; b++)
//                 line[b].c = c_as_uint;
            __stosd((unsigned long*)line, c.c, w);
            line += bmpWidth;
        }
    }
    else
    {
        for (int a = y1; a <= y2; a++)
            HLineUnclipped(bmp, x1, a, w, c);
    }
}


void RectOutline(DfBitmap *bmp, int x, int y, unsigned w, unsigned h, DfColour c)
{
    HLine(bmp, x, y,     w, c);
    HLine(bmp, x, y+h-1, w, c);
    VLine(bmp, x, y,     h, c);
    VLine(bmp, x+w-1, y, h, c);
}


void CircleOutline(DfBitmap *bmp, int x0, int y0, unsigned radius, DfColour c)
{
    // This is the standard Bresenham or Mid-point circle algorithm
    int x = radius;
    int y = 0;
    int radiusError = 1 - x;

    do
    {
        PutPix(bmp, x0 + x, y0 + y, c);
        PutPix(bmp, x0 + y, y0 + x, c);
        PutPix(bmp, x0 - x, y0 + y, c);
        PutPix(bmp, x0 - y, y0 + x, c);
        PutPix(bmp, x0 - x, y0 - y, c);
        PutPix(bmp, x0 - y, y0 - x, c);
        PutPix(bmp, x0 + x, y0 - y, c);
        PutPix(bmp, x0 + y, y0 - x, c);
        y++;
        if (radiusError < 0)
        {
            radiusError += 2 * y + 1;
        }
        else
        {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    } while (x >= y);
}


void CircleFill(DfBitmap *bmp, int x0, int y0, unsigned radius, DfColour c)
{
    int x = radius;
    int y = 0;
    int radiusError = 1 - x;

    // Iterate through all points along an arc of 1/8th of the circle, drawing
    // horizontal chords for all 4 symmetries at the same time.
    do 
    {
        HLine(bmp, x0 - x, y0 - y, x*2+1, c);
        HLine(bmp, x0 - x, y0 + y, x*2+1, c);
        HLine(bmp, x0 - y, y0 - x, y*2+1, c);
        HLine(bmp, x0 - y, y0 + x, y*2+1, c);

        y++;
        if (radiusError < 0)
        {
            radiusError += 2 * y + 1;
        }
        else
        {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    } while (x >= y);
}


static void EllipsePlotPoints(DfBitmap *bmp, int x0, int y0, int x, int y, DfColour c)
{
    PutPix(bmp, x0 + x, y0 + y, c);
    PutPix(bmp, x0 - x, y0 + y, c);
    PutPix(bmp, x0 + x, y0 - y, c);
    PutPix(bmp, x0 - x, y0 - y, c);
}


void EllipseOutline(DfBitmap *bmp, int x0, int y0, unsigned rx, unsigned ry, DfColour c)
{
    int rxSqrd = rx * rx;
    int rySqrd = ry * ry;
    int x = 0;
    int y = ry;
    int px = 0;
    int py = 2 * rxSqrd * y;

    // Plot the initial point in each quadrant.
    EllipsePlotPoints(bmp, x0, y0, x, y, c);

    // Region 1
    int p = rySqrd - (rxSqrd * ry) + RoundToInt(0.25 * (double)rxSqrd);
    while (px < py) 
    {
        x++;
        px += 2 * rySqrd;
        if (p < 0)
            p += rySqrd + px;
        else 
        {
            y--;
            py -= 2 * rxSqrd;
            p += rySqrd + px - py;
        }

        EllipsePlotPoints(bmp, x0, y0, x, y, c);
    }

    // Region 2
    p = RoundToInt(rySqrd * (x+0.5) * (x+0.5) + rxSqrd * (y-1) * (y-1) - rxSqrd * rySqrd);
    while (y > 0) 
    {
        y--;
        py -= 2 * rxSqrd;
        if (p > 0)
            p += rxSqrd - py;
        else 
        {
            x++;
            px += 2 * rySqrd;
            p += rxSqrd - py + px;
        }
        
        EllipsePlotPoints(bmp, x0, y0, x, y, c);
    }
}


static void EllipsePlotHlines(DfBitmap *bmp, int x0, int y0, int x, int y, DfColour c)
{
    HLine(bmp, x0 - x, y0 + y, x * 2 + 1, c);
    HLine(bmp, x0 - x, y0 - y, x * 2 + 1, c);
}


void EllipseFill(DfBitmap *bmp, int x0, int y0, unsigned rx, unsigned ry, DfColour c)
{
    int rxSqrd = rx * rx;
    int rySqrd = ry * ry;
    int x = 0;
    int y = ry;
    int px = 0;
    int py = 2 * rxSqrd * y;

    // Plot the initial point in each quadrant.
    EllipsePlotHlines(bmp, x0, y0, x, y, c);

    // Region 1
    int p = rySqrd - (rxSqrd * ry) + RoundToInt(0.25 * (double)rxSqrd);
    while (px < py) 
    {
        x++;
        px += 2 * rySqrd;
        if (p < 0)
            p += rySqrd + px;
        else 
        {
            y--;
            py -= 2 * rxSqrd;
            p += rySqrd + px - py;
        }

        EllipsePlotHlines(bmp, x0, y0, x, y, c);
    }

    // Region 2
    p = RoundToInt(rySqrd * (x+0.5) * (x+0.5) + rxSqrd * (y-1) * (y-1) - rxSqrd * rySqrd);
    while (y > 0) 
    {
        y--;
        py -= 2 * rxSqrd;
        if (p > 0)
            p += rxSqrd - py;
        else 
        {
            x++;
            px += 2 * rySqrd;
            p += rxSqrd - py + px;
        }

        EllipsePlotHlines(bmp, x0, y0, x, y, c);
    }
}


void MaskedBlit(DfBitmap *destBmp, int x, int y, DfBitmap *srcBmp)
{
	if ((unsigned)x > destBmp->width || (unsigned)y > destBmp->height)
		return;

	// Reduce the amount of the src bitmap to copy, if it would go 
	// beyond the edges of the dest bitmap
	unsigned w = srcBmp->width;
	if (x + w > destBmp->width)
		w = destBmp->width - x;
	unsigned h = srcBmp->height;
	if (y + h > destBmp->height)
		h = destBmp->height - y;

	for (unsigned sy = 0; sy < h; sy++)
	{
		DfColour *srcLine = GetLine(srcBmp, sy);
		DfColour *destLine = GetLine(destBmp, y + sy) + x;
		for (unsigned i = 0; i < w; i++)
		{
			if (srcLine[i].a > 0)
				destLine[i] = srcLine[i];
		}
	}
}


void QuickBlit(DfBitmap *destBmp, unsigned x, unsigned y, DfBitmap *srcBmp)
{
    if (x > destBmp->width || y > destBmp->height)
        return;

    // Reduce the amount of the src bitmap to copy, if it would go 
    // beyond the edges of the dest bitmap
    unsigned w = srcBmp->width;
    if (x + w > destBmp->width)
        w = destBmp->width - x;
    unsigned h = srcBmp->height;
    if (y + h > destBmp->height)
        h = destBmp->height - y;

    for (unsigned sy = 0; sy < h; sy++)
    {
        DfColour *srcLine = GetLine(srcBmp, sy);
        DfColour *destLine = GetLine(destBmp, y + sy) + x;
        memcpy(destLine, srcLine, w * sizeof(DfColour));
    }
}


void ScaleDownBlit(DfBitmap *dest, unsigned x, unsigned y, unsigned scale, DfBitmap *src)
{
    unsigned outW = src->width / scale;
    unsigned outH = src->height / scale;
    unsigned scaleSqrd = scale * scale;

    for (unsigned dy = 0; dy < outH; dy++)
    {
        for (unsigned dx = 0; dx < outW; dx++)
        {
            int r = 0, g = 0, b = 0, a = 0;
            for (unsigned sy = dy * scale; sy < (dy+1) * scale; sy++)
            {
                for (unsigned sx = dx * scale; sx < (dx+1) * scale; sx++)
                {
                    DfColour srcPix = GetPix(src, sx, sy);
                    r += srcPix.r;
                    g += srcPix.g;
                    b += srcPix.b;
                    a += srcPix.a;
                }
            }

            r /= scaleSqrd;
            g /= scaleSqrd;
            b /= scaleSqrd;
            a /= scaleSqrd;

            PutPix(dest, x + dx, y + dy, Colour(r,g,b,a));
        }
    }
}



void ScaleUpBlit(DfBitmap *destBmp, unsigned x, unsigned y, unsigned scale, DfBitmap *srcBmp)
{
    for (unsigned sy = 0; sy < srcBmp->height; sy++)
    {
        for (unsigned j = 0; j < scale; j++)
        {
            DfColour *srcPixel = srcBmp->lines[sy];
            DfColour *destPixel = destBmp->lines[y + sy * scale + j] + x * scale;

            for (unsigned sx = 0; sx < srcBmp->width; sx++)
            {
                for (unsigned i = 0; i < scale; i++)
                {
                    *destPixel = *srcPixel;
                    destPixel++;
                }

                srcPixel++;
            }
        }
    }

    for (unsigned sy = 0; sy < srcBmp->height; sy++)
        HLine(destBmp, x, y + sy * scale, srcBmp->width * scale, Colour(0,0,208));

    for (unsigned sx = 0; sx < srcBmp->width; sx++)
        VLine(destBmp, x + sx * scale, y, srcBmp->height * scale, Colour(0,0,208));
}


void BitmapDownsample(DfBitmap *src_bmp, DfBitmap *dst_bmp)
{
    int w1 = src_bmp->width;
    int h1 = src_bmp->height;

    int w2 = dst_bmp->width;
    int h2 = dst_bmp->height;

    // Both buffers must be in ARGB format, and a scanline should be w*4 bytes.

    // NOTE: THIS WILL OVERFLOW for really major downsizing (2800x2800 to 1x1 or more) 
    // (2800 ~ sqrt(2^23)) - for a lazy fix, just call this in two passes.

    // arbitrary resize.
    unsigned int *dsrc  = &src_bmp->pixels->c;
    unsigned int *ddest = &dst_bmp->pixels->c;

    // If too many input pixels map to one output pixel, our 32-bit accumulation values
    // could overflow - so, if we have huge mappings like that, cut down the weights:
    //    256 max color value
    //   *256 weight_x
    //   *256 weight_y
    //   *256 (16*16) maximum # of input pixels (x,y) - unless we cut the weights down...
    int weight_shift = 0;
    float source_texels_per_out_pixel = ( (w1/(float)w2 + 1) * (h1/(float)h2 + 1) );
    float weight_per_pixel = source_texels_per_out_pixel * 256 * 256;  //weight_x * weight_y
    float accum_per_pixel = weight_per_pixel*256; //color value is 0-255
    float weight_div = accum_per_pixel / 4294967000.0f;
    if (weight_div > 1)
        weight_shift = (int)ceilf( logf((float)weight_div)/logf(2.0f) );
    weight_shift = std::min(15, weight_shift);  // this could go to 15 and still be ok.

    float fh = 256*h1/(float)h2;
    float fw = 256*w1/(float)w2;

    // FOR EVERY OUTPUT PIXEL
    for (int y2=0; y2<h2; y2++)
    {   
        // find the y-range of input pixels that will contribute:
        int y1a = (int)((y2  )*fh); 
        int y1b = (int)((y2+1)*fh); 
        y1b = std::min(y1b, 256*h1 - 1);
        int y1c = y1a >> 8;
        int y1d = y1b >> 8;

        for (int x2=0; x2<w2; x2++)
        {
            // find the x-range of input pixels that will contribute:
            // find the x-range of input pixels that will contribute:
            int x1a = (int)((x2)*fw);
            int x1b = (int)((x2 + 1)*fw);
            x1b = std::min(x1b, 256 * w1 - 1);
            int x1c = x1a >> 8;
            int x1d = x1b >> 8;

            // ADD UP ALL INPUT PIXELS CONTRIBUTING TO THIS OUTPUT PIXEL:
            unsigned int r=0, g=0, b=0, a=0;
            for (int y=y1c; y<=y1d; y++)
            {
                unsigned int weight_y = 256;
                if (y1c != y1d) 
                {
                    if (y==y1c)
                        weight_y = 256 - (y1a & 0xFF);
                    else if (y==y1d)
                        weight_y = (y1b & 0xFF);
                }

                unsigned int *dsrc2 = &dsrc[y*w1 + x1c];
                for (int x=x1c; x<=x1d; x++)
                {
                    unsigned int weight_x = 256;
                    if (x1c != x1d) 
                    {
                        if (x==x1c)
                            weight_x = 256 - (x1a & 0xFF);
                        else if (x==x1d)
                            weight_x = (x1b & 0xFF);
                    }

                    unsigned int c = *dsrc2++;//dsrc[y*w1 + x];
                    unsigned int r_src = (c    ) & 0xFF;
                    unsigned int g_src = (c>> 8) & 0xFF;
                    unsigned int b_src = (c>>16) & 0xFF;
                    unsigned int w = (weight_x * weight_y) >> weight_shift;
                    r += r_src * w;
                    g += g_src * w;
                    b += b_src * w;
                    a += w;
                }
            }

            // write results
            unsigned int c = ((r/a)) | ((g/a)<<8) | ((b/a)<<16);
            *ddest++ = c;//ddest[y2*w2 + x2] = c;
        }
    }
}

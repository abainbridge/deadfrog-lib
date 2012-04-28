#include <math.h>
#include <stdlib.h>
#include "lib/gfx/bitmap.h"
#include "lib/gfx/rgba_colour.h"
#include "lib/debug_utils.h"


BitmapRGBA *CreateBitmapRGBA(unsigned width, unsigned height)
{
	BitmapRGBA *bmp = new BitmapRGBA;
	bmp->width = width;
	bmp->height = height;
	bmp->pixels = new RGBAColour[width * height];
	bmp->lines = new RGBAColour *[height];

	DebugAssert(bmp->pixels && bmp->lines);

	for (unsigned y = 0; y < height; ++y)
		bmp->lines[y] = &bmp->pixels[y * width];

    return bmp;
}


void DeleteBitmapRGBA(BitmapRGBA *bmp)
{
	bmp->width = -1;
	bmp->height = -1;
	delete [] bmp->pixels;
	delete [] bmp->lines;
	bmp->pixels = NULL;
	bmp->lines = NULL;
}


void ClearBitmap(BitmapRGBA *bmp, RGBAColour colour)
{
    RectFill(bmp, 0, 0, bmp->width, bmp->height, colour);
}


RGBAColour GetPlotUnclipped(BitmapRGBA *bmp, unsigned x, unsigned y)
{
	return bmp->lines[y][x];
}


void Plot(BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour colour)
{
	if (x >= bmp->width || y >= bmp->height)
		return;

	bmp->lines[y][x] = colour;
}


RGBAColour GetPlot(BitmapRGBA *bmp, unsigned x, unsigned y)
{
	if (x >= bmp->width || y >= bmp->height)
		return g_colourBlack;

	return bmp->lines[y][x]; 
}


void HLineUnclipped(BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c)
{
    RGBAColour *line = bmp->lines[y];
    for (unsigned i = 0; i < len; i++)
        line[x + i] = c;
}


void HLine(BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c)
{
    // Clip against top and bottom of bmp
    if (unsigned(y) > bmp->height)
        return;

    // Clip against left
    if (x < 0)
    {
        len += x;
        x = 0;
    }

    // Clip against right
    if (x + len > int(bmp->width))
    {
        if (x > (int)bmp->width)
            return;

        len = bmp->width - x;
    }

    HLineUnclipped(bmp, x, y, len, c);
}


void VLineUnclipped(BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c)
{
    for (unsigned i = 0; i < len; i++)
        bmp->lines[y][x + i] = c;
}


void VLine(BitmapRGBA *bmp, int x, int y, unsigned len, RGBAColour c)
{
    // Clip against left and right of bmp
    if (unsigned(x) > bmp->width)
        return;

    // Clip against top
    if (y < 0)
    {
        len += y;
        y = 0;
    }

    // Clip against bottom
    if (y + len > int(bmp->height))
    {
        if (y > (int)bmp->height)
            return;

        len = bmp->height - y;
    }

    VLineUnclipped(bmp, x, y, len, c);
}


void DrawLine(BitmapRGBA *bmp, int x1, int y1, int x2, int y2, RGBAColour colour)
{
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
        return VLine(bmp, x1, y1, yDelta, colour);
    if (yDelta == 0)
    {
        if (xDelta > 0)
            return HLine(bmp, x1, y1, xDelta, colour);
        else
            return HLine(bmp, x2, y1, -xDelta, colour);
    }

    // Clip first end of line
    if ((unsigned)x1 > bmp->width || (unsigned)y1 > bmp->height ||
        (unsigned)x2 > bmp->width || (unsigned)y2 > bmp->height)
    {
        double slope = (double)yDelta / (double)xDelta;

        // Clip first end against left and right of bitmap
        if (x1 < 0)
        {
            y1 -= x1 * slope + 0.5;
            x1 = 0;
        }
        else if ((unsigned)x1 > bmp->width)
        {
            y1 += ((int)bmp->width - (x1 + 1)) * slope + 0.5;
            x1 = bmp->width - 1;
        }

        // Clip first end against top and bottom of bitmap
        if (y1 < 0)
        {
            x1 += (y1 / slope) + 0.5;
            y1 = 0;
        }
        else if ((unsigned)y1 > bmp->height)
        {
            return; // The entire line is off the bmp
        }

        // Clip second end against left and right of bitmap
        if (x2 < 0)
        {
            y2 -= x2 * slope + 0.5;
            x2 = 0;
        }
        else if ((unsigned)x2 > bmp->width)
        {
            y2 += ((int)bmp->width - (x2 + 1)) * slope + 0.5;
            x2 = bmp->width - 1;
        }

        // Clip second end against top and bottom of bitmap
        if (y2 < 0)
        {
            return; // The entire line is off the bmp
        }
        else if ((unsigned)y2 > bmp->height)
        {
            x2 += (y2 / slope) + 0.5;
            y2 = bmp->height - 1;
        }

        xDelta = x2 - x1;
        yDelta = y2 - y1;
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
        for (int i = 0; i < xDelta; i++)
            PlotUnclipped(bmp, x1 + i * xAdvance, y1 + i, colour);
        return;
    }

    RGBAColour *pixel = *(bmp->lines + y1) + x1;

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
        pixel += bmp->width;

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
            pixel += bmp->width;
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
        int lineInc = xAdvance * bmp->width;
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


void RectFill(BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c)
{
    if (unsigned(x) + w > bmp->width)
    {
        if (unsigned(x) > bmp->width)
            return;
        w = bmp->width - x;
        x = 0;
    }

    if (unsigned(y) + h > bmp->height)
    {
        if (unsigned(y) > bmp->height)
            return;
        h = bmp->height - y;
        y = 0;
    }

    for (unsigned a = 0; a < h; a++)
    {
        RGBAColour *line = bmp->lines[y + a] + x;
        for (unsigned b = 0; b < w; b++)
            line[b] = c;
    }
}


void RectOutline(BitmapRGBA *bmp, int x, int y, unsigned w, unsigned h, RGBAColour c)
{
    HLine(bmp, x, y,     w, c);
    HLine(bmp, x, y+h-1, w, c);
    VLine(bmp, x, y,     h, c);
    VLine(bmp, x+w-1, y, h, c);
}


void QuickBlit(BitmapRGBA *destBmp, unsigned x, unsigned y, BitmapRGBA *srcBmp)
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
		RGBAColour *srcLine = srcBmp->lines[sy];
		RGBAColour *destLine = destBmp->lines[y + sy] + x;
		for (unsigned i = 0; i < w; i++)
		{
			if (CompA(srcLine[i]) > 0)
				destLine[i] = srcLine[i];
		}
	}
}

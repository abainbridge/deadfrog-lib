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


RGBAColour DfGetPixel(BitmapRGBA *bmp, unsigned x, unsigned y)
{
	return bmp->lines[y][x];
}


void PutPixelClipped(BitmapRGBA *bmp, unsigned x, unsigned y, RGBAColour colour)
{
	if (x >= bmp->width || y >= bmp->height)
		return;

	bmp->lines[y][x] = colour;
}


RGBAColour GetPixelClipped(BitmapRGBA *bmp, unsigned x, unsigned y)
{
	if (x >= bmp->width || y >= bmp->height)
		return g_colourBlack;

	return bmp->lines[y][x]; 
}


void DrawLine(BitmapRGBA *bmp, int _x1, int _y1, 
              int _x2, int _y2, RGBAColour colour)
{
    int x1 = _x1;
    int x2 = _x2;
    int y1 = _y1;
    int y2 = _y2;
    int numSteps = sqrtf( (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1) );
    float dx = float(x2-x1) / (float) numSteps;
    float dy = float(y2-y1) / (float) numSteps;

    float currentX = x1;
    float currentY = y1;

    for( int i = 0; i < numSteps; ++i )
    {
        PutPixelClipped(bmp, currentX, currentY, colour);

        currentX += dx;
        currentY += dy;
    }
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

    RGBAColour *line = bmp->lines[y];
    for (unsigned i = 0; i < len; i++)
        line[x + i] = c;
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

    for (unsigned i = 0; i < len; i++)
        bmp->lines[y][x + i] = c;
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

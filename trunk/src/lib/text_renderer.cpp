#include "text_renderer.h"

#include "lib/bitmap.h"
#include "lib/rgba_colour.h"
#include "lib/common.h"
#include "lib/window_manager.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


// ****************************************************************************
// Glyph
// ****************************************************************************

typedef signed char rleBufType;

class Glyph
{
public:
	int m_width;
	int m_startY;
	rleBufType *m_rlePixels;

	Glyph(int w, int h)
	{
		m_startY = 0;
		m_width = w;
	}
};


// ****************************************************************************
// Public Functions
// ****************************************************************************

TextRenderer *CreateTextRenderer(char const *fontName, int size)
{
    TextRenderer *tr = new TextRenderer;

    // Create a memory bitmap for GDI to render the font into
	HDC winDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    HDC memDC = CreateCompatibleDC(winDC);
    HBITMAP memBmp = CreateCompatibleBitmap(memDC, size * 2, size * 2);
	HBITMAP winBmp = (HBITMAP)SelectObject(memDC, memBmp);

	int scaledSize = -MulDiv(size, GetDeviceCaps(memDC, LOGPIXELSY), 72);

 	HFONT fontHandle = CreateFont(scaledSize, 0, 0, 0, FW_NORMAL, false, false, false, 
 							  ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
							  NONANTIALIASED_QUALITY, DEFAULT_PITCH,
 							  fontName);
	ReleaseAssert(fontHandle != NULL, "Couldn't find Windows font '%s'", fontName);

	// Ask GDI about the font fixed-widthness
	SelectObject(memDC, fontHandle); 
	TEXTMETRIC textMetrics;
	GetTextMetrics(memDC, &textMetrics);
	tr->charHeight = textMetrics.tmHeight;
	tr->maxCharWidth = textMetrics.tmAveCharWidth;
	tr->fixedWidth = (textMetrics.tmAveCharWidth == textMetrics.tmMaxCharWidth);

	// Ask GDI about the name of the font
	char nameOfFontWeGot[256];
	GetTextFace(memDC, 256, nameOfFontWeGot);
	ReleaseWarn(strnicmp(nameOfFontWeGot, fontName, 255) == 0,
		"Attempt to load font '%s' failed.\n"
		"'%s' will be used instead.", 
		fontName, nameOfFontWeGot);

	// Determine the size of the font
	tr->maxCharWidth = 0;
	tr->charHeight = 0;
	for (int i = 0; i < 256; i++)
	{
		char buf[] = {i};
		SIZE size;
		GetTextExtentPoint32(memDC, buf, 1, &size);
		if (size.cx > tr->maxCharWidth)
			tr->maxCharWidth = size.cx;
		if (size.cy > tr->maxCharWidth)
			tr->charHeight = size.cy;
	}

	// Run-length encode each ASCII character
    // Format is:
    //   1 means increment X coord by 1 then draw a pixel
    //   2 means increment X coord by 2 then draw a pixel
    //   etc
    //   -1 means end of row of pixels
    //   -2 means end of glyph
	rleBufType *rleBuf = new rleBufType [tr->charHeight * tr->maxCharWidth];	// temp working space
	for (int i = 0; i < 256; i++)
	{
		// Clear a space in the GDI bitmap where we will draw the character
		SetTextColor(memDC, RGB(255,255,255));
		RECT rect;
		rect.left = 0;
		rect.right = tr->maxCharWidth;
		rect.top = 0;
		rect.bottom = tr->charHeight;
		SetBkColor(memDC, 0);
		ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, "", 0, 0); // Yes I'm using textout to fill a rectangle!

		// Ask GDI to draw the character
		char buf[] = {i};
		TextOut(memDC, 0, 0, buf, 1);

        // Read back what GDI put in the bitmap and construct a run-length encoding
		memset(rleBuf, 0, tr->charHeight * tr->maxCharWidth);
		rleBufType *pixel = rleBuf;
		for (int y = 0; y < tr->charHeight; y++)
		{
			int blankCount = 1;
			bool lineEmpty = true;
			for (int x = 0; x < tr->maxCharWidth; x++)
			{
				int c = GetPixel(memDC, x, y);
				if (c)
				{
					lineEmpty = false;
					*pixel = blankCount;
					pixel++;
					blankCount = 1;
				}
				else
				{
					blankCount++;
				}
			}

			*pixel = -1;
			pixel++;
		}

        SIZE size;
        GetTextExtentPoint32(memDC, buf, 1, &size);
        if (size.cx > tr->maxCharWidth)
            tr->maxCharWidth = size.cx;
        if (size.cy > tr->maxCharWidth)
            tr->charHeight = size.cy;

        // Create the glyph to store the RLE
		Glyph *glyph = new Glyph(size.cx, tr->charHeight);
		tr->glyphs[i] = glyph;

		// Optimise the RLE by removing blank lines at the start of the glyph
 		rleBufType *betterStart = rleBuf;
 		while (*betterStart == -1)
 		{
 			betterStart++;
			glyph->m_startY++;
 		}

		// Optimise the RLE by removing blank lines at the end of the glyph
		rleBufType *betterEnd = pixel;
		if (glyph->m_startY == tr->charHeight)
		{
			// Glyph was entirely blank. Handle this as a special case
			betterStart--;
			*betterStart = -2;
		}
		else
		{
			while (betterEnd > betterStart && 
                   *(betterEnd-1) == -1    &&
                   *(betterEnd-2) == -1)
				betterEnd--;
			*(betterEnd -1) = -2;
		}

		// Copy the optimised RLE into the glyph
		int rleLen = betterEnd - betterStart;
		glyph->m_rlePixels = new rleBufType [rleLen];
		memcpy(glyph->m_rlePixels, betterStart, rleLen * sizeof(rleBufType));
	}

	delete [] rleBuf;

	// Release the GDI resources
    DeleteDC(winDC);
    DeleteDC(memDC);
	DeleteObject(fontHandle);
	DeleteObject(memBmp);
	DeleteObject(winBmp);

    return tr;
}


int DrawTextSimple(TextRenderer *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int _y, char const *text, int maxChars)
{
    int x = _x;
    int y = _y;

    if (x < 0 || y < 0 || (y + tr->charHeight) > (int)bmp->height)
        return 0;	// TODO implement slow but accurate clipping render

    while (*text != '\0')
    {
        if (x + tr->maxCharWidth > (int)bmp->width)
            break;

        Glyph *glyph = tr->glyphs[(unsigned char)*text];
        rleBufType *rleBuf = glyph->m_rlePixels;

        RGBAColour **dstLine = bmp->lines + y + glyph->m_startY;
        int lineOffset = 0;

        while (1)
        {
            if (*rleBuf == -1)
            {
                lineOffset = 0;
                dstLine++;
            }
            else if (*rleBuf == -2)
            {
                break;
            }
            else
            {
                lineOffset += *rleBuf;
                (*dstLine)[x+ lineOffset] = col;
            }

            rleBuf++;
        }

        x += glyph->m_width;
        text++;
    }

    return _x - x;
}


int DrawTextLeft(TextRenderer *tr, RGBAColour c, BitmapRGBA *bmp, int _x, int _y, char const *_text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);
    return DrawTextSimple(tr, c, bmp, _x, _y, buf);
}


int DrawTextRight(TextRenderer *tr, RGBAColour c, BitmapRGBA *bmp, int _x, int _y, char const *_text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, _x - width, _y, buf);
}


int DrawTextCentre(TextRenderer *tr, RGBAColour c, BitmapRGBA *bmp, int _x, int _y, char const *_text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _text);
    vsprintf(buf, _text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, _x - width/2, _y, buf);
}


int GetTextWidth(TextRenderer *tr, char const *text, int len)
{
	len = min((int)strlen(text), len);
	if (tr->fixedWidth)
	{
		return len * tr->maxCharWidth;
	}
	else
	{
        int width = 0;
        for (; len; len--)
            width += tr->glyphs[text[len]]->m_width;

		return width;
	}
}

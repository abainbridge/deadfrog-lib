#include "text_renderer.h"

#include "lib/bitmap.h"
#include "lib/common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


// ****************************************************************************
// Glyph
// ****************************************************************************

typedef struct
{
    unsigned startX;
    unsigned startY;
    unsigned runLen;
} EncodedRun;


class Glyph
{
public:
    unsigned m_width;
    int m_numRuns;
    EncodedRun *m_pixelRuns;

	Glyph(int w)
	{
		m_width = w;
	}
};


// ****************************************************************************
// Public Functions
// ****************************************************************************

TextRenderer *CreateTextRenderer(char const *fontName, int size)
{
    TextRenderer *tr = new TextRenderer;
    memset(tr, 0, sizeof(TextRenderer));

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

    // Allocate enough EncodedRuns to store the worst case for a glyph of this size.
    // Worse case is if the whole glyph is encoded as runs of one pixel. There has
    // to be a gap of one pixel between each run, so there can only be half as many
    // runs as there are pixels.
    EncodedRun *tempRuns = new EncodedRun [tr->charHeight * tr->maxCharWidth / 2];

    // Setup stuff needed to render text with GDI
    SetTextColor(memDC, RGB(255,255,255));
    SetBkColor(memDC, 0);
    RECT rect = { 0, tr->maxCharWidth, 0, tr->charHeight };

    // Run-length encode each ASCII character
    for (int i = 0; i < 256; i++)
	{
        char buf[] = {i};

        // Get the size of this glyph
        SIZE size;
        GetTextExtentPoint32(memDC, buf, 1, &size);
        if (size.cx > tr->maxCharWidth)
            tr->maxCharWidth = size.cx;
        if (size.cy > tr->maxCharWidth)
            tr->charHeight = size.cy;

        // Ask GDI to draw the character
		ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, buf, 1, 0);

        // Read back what GDI put in the bitmap and construct a run-length encoding
		memset(tempRuns, 0, tr->charHeight * tr->maxCharWidth / 2);
		EncodedRun *run = tempRuns;
        for (int y = 0; y < tr->charHeight; y++)
		{
            int x = 0;
            while (x < tr->maxCharWidth)
            {
                // Skip blank pixels
                while (!GetPixel(memDC, x, y))
                {
                    x++;
                    if (x >= tr->maxCharWidth)
                        break;
                }

                // Have we got to the end of the line?
                if (x >= tr->maxCharWidth)
                    continue;

                run->startX = x;
                run->startY = y;
                run->runLen = 0;

                // Count non-blank pixels
                while (GetPixel(memDC, x, y))
                {
                    x++;
                    run->runLen++;
                    if (x >= tr->maxCharWidth)
                        break;
                }

                run++;
            }
		}

        // Create the glyph to store the encoded runs we've made
		Glyph *glyph = new Glyph(size.cx);
		tr->glyphs[i] = glyph;

		// Copy the runs into the glyph
		glyph->m_numRuns = run - tempRuns;
		glyph->m_pixelRuns = new EncodedRun [glyph->m_numRuns];
		memcpy(glyph->m_pixelRuns, tempRuns, glyph->m_numRuns * sizeof(EncodedRun));
	}

	delete [] tempRuns;

	// Release the GDI resources
    DeleteDC(winDC);
    DeleteDC(memDC);
	DeleteObject(fontHandle);
	DeleteObject(memBmp);
	DeleteObject(winBmp);

    return tr;
}


int DrawTextSimple(TextRenderer *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int y, char const *text, int maxChars)
{
    int x = _x;

    if (x < 0 || y < 0 || (y + tr->charHeight) > (int)bmp->height)
        return 0;	// TODO implement slow but accurate clipping render

    while (*text != '\0')
    {
        if (x + tr->maxCharWidth > (int)bmp->width)
            break;

        Glyph *glyph = tr->glyphs[(unsigned char)*text];
        EncodedRun *rleBuf = glyph->m_pixelRuns;
        for (int i = 0; i < glyph->m_numRuns; i++)
        {
            RGBAColour *dstLine = bmp->lines[y + rleBuf->startY] + rleBuf->startX + x;
            for (unsigned i = 0; i < rleBuf->runLen; i++)
                dstLine[i] = col;
            rleBuf++;
        }

        x += glyph->m_width;
        text++;
    }

    return _x - x;
}


int DrawTextLeft(TextRenderer *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimple(tr, c, bmp, x, y, buf);
}


int DrawTextRight(TextRenderer *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, x - width, y, buf);
}


int DrawTextCentre(TextRenderer *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, x - width/2, y, buf);
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

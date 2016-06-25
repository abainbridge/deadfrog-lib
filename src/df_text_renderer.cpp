#include "df_text_renderer.h"

#include "df_bitmap.h"
#include "df_common.h"

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
// Global variables
// ****************************************************************************

TextRenderer *g_defaultTextRenderer = NULL;


// ****************************************************************************
// Public Functions
// ****************************************************************************

static bool GetPixelFromBuffer(unsigned *buf, int w, int h, int x, int y)
{
    return buf[(h - y - 1) * w + x] > 0;
}


TextRenderer *CreateTextRenderer(char const *fontName, int size, int weight)
{
    if (size < 4 || size > 1000 || weight < 1 || weight > 9)
        return NULL;

    TextRenderer *tr = new TextRenderer;
    memset(tr, 0, sizeof(TextRenderer));

    // Get the font from GDI
    HDC winDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    HDC memDC = CreateCompatibleDC(winDC);
    int scaledSize = -MulDiv(size, GetDeviceCaps(memDC, LOGPIXELSY), 72);
    HFONT fontHandle = CreateFont(scaledSize, 0, 0, 0, weight * 100, false, false, false, 
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, DEFAULT_PITCH,
        fontName);
    ReleaseAssert(fontHandle != NULL, "Couldn't find Windows font '%s'", fontName);

    // Ask GDI about the font size and fixed-widthness
    SelectObject(memDC, fontHandle); 
    TEXTMETRIC textMetrics;
    GetTextMetrics(memDC, &textMetrics);
    tr->charHeight = textMetrics.tmHeight;
    tr->maxCharWidth = textMetrics.tmMaxCharWidth;
    tr->fixedWidth = (textMetrics.tmAveCharWidth == textMetrics.tmMaxCharWidth);

    // Ask GDI about the name of the font
    char nameOfFontWeGot[256];
    GetTextFace(memDC, 256, nameOfFontWeGot);
    ReleaseWarn(strnicmp(nameOfFontWeGot, fontName, 255) == 0,
        "Attempt to load font '%s' failed.\n"
        "'%s' will be used instead.", 
        fontName, nameOfFontWeGot);

    // Create an off-screen bitmap
    BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biHeight = tr->charHeight;
    bmpInfo.bmiHeader.biWidth = tr->maxCharWidth;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    UINT *gdiPixels = 0;
    HBITMAP memBmp = CreateDIBSection(memDC, (BITMAPINFO *)&bmpInfo, DIB_RGB_COLORS, (void **)&gdiPixels, NULL, 0);
	HBITMAP prevBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // Allocate enough EncodedRuns to store the worst case for a glyph of this size.
    // Worst case is if the whole glyph is encoded as runs of one pixel. There has
    // to be a gap of one pixel between each run, so there can only be half as many
    // runs as there are pixels.
    unsigned tempRunsSize = tr->charHeight * (tr->maxCharWidth / 2 + 1);
    EncodedRun *tempRuns = new EncodedRun [tempRunsSize];

    // Setup stuff needed to render text with GDI
    SetTextColor(memDC, RGB(255,255,255));
    SetBkColor(memDC, 0);
    RECT rect = { 0, 0, tr->maxCharWidth, tr->charHeight };

    // Run-length encode each ASCII character
    for (int i = 0; i < 256; i++)
	{
        char buf[] = {(char)i};

        // Get the size of this glyph
        SIZE glyphSize;
        GetTextExtentPoint32(memDC, buf, 1, &glyphSize);
//         if (glyphSize.cx > tr->maxCharWidth)
//             tr->maxCharWidth = glyphSize.cx;
//         if (glyphSize.cy > tr->charHeight)
//             tr->charHeight = glyphSize.cy;

        // Ask GDI to draw the character
		ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, buf, 1, 0);

        // Read back what GDI put in the bitmap and construct a run-length encoding
		memset(tempRuns, 0, tempRunsSize);
		EncodedRun *run = tempRuns;
        for (int y = 0; y < tr->charHeight; y++)
		{
            int x = 0;
            while (x < tr->maxCharWidth)
            {
                // Skip blank pixels
                while (!GetPixelFromBuffer(gdiPixels, tr->maxCharWidth, tr->charHeight, x, y))
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
                while (GetPixelFromBuffer(gdiPixels, tr->maxCharWidth, tr->charHeight, x, y))
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
		Glyph *glyph = new Glyph(glyphSize.cx);
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
	DeleteObject(prevBmp);

    return tr;
}


static int DrawTextSimpleClipped(TextRenderer *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int y, char const *text)
{
    int x = _x;
    RGBAColour *startRow = bmp->pixels + y * bmp->width;
    int width = bmp->width;

    if (y + tr->charHeight < 0 || y > (int)bmp->height)
        return 0;

    while (*text != '\0')
    {
        if (x + tr->maxCharWidth > (int)bmp->width)
            break;

        unsigned char c = *((unsigned char const *)text);
        Glyph *glyph = tr->glyphs[c]; 
        EncodedRun *rleBuf = glyph->m_pixelRuns;
        for (int i = 0; i < glyph->m_numRuns; i++)
        {
            int y3 = y + rleBuf->startY;
            if (y3 >= 0 && y3 < (int)bmp->height)
            {
                RGBAColour *thisRow = startRow + rleBuf->startY * width;
                for (unsigned i = 0; i < rleBuf->runLen; i++)
                {
                    int x3 = x + rleBuf->startX + i;
                    if (x3 >= 0 && x3 < width)
                        thisRow[x3] = col;
                }
            }
            rleBuf++;
        }

        x += glyph->m_width;
        text++;
    }

    return _x - x;
}


int DrawTextSimple(TextRenderer *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int y, char const *text)
{
    int x = _x;
    int width = bmp->width;

    if (x < 0 || y < 0 || (y + tr->charHeight) > (int)bmp->height)
        return DrawTextSimpleClipped(tr, col, bmp, _x, y, text);

    RGBAColour *startRow = bmp->pixels + y * bmp->width;
    while (*text != '\0')
    {
        unsigned char c = *((unsigned char const *)text);
        if (x + tr->glyphs[c]->m_width > (int)bmp->width)
            break;

        // Copy the glyph onto the stack for better cache performance. This increased the 
        // performance from 13.8 to 14.4 million chars per second. Madness.
        Glyph glyph = *tr->glyphs[(unsigned char)*text]; 
        EncodedRun *rleBuf = glyph.m_pixelRuns;
        for (int i = 0; i < glyph.m_numRuns; i++)
        {
            RGBAColour *startPixel = startRow + rleBuf->startY * width + rleBuf->startX + x;

            // Loop unrolled for speed
            switch (rleBuf->runLen)
            {
            case 8: startPixel[7] = col;
            case 7: startPixel[6] = col;
            case 6: startPixel[5] = col;
            case 5: startPixel[4] = col;
            case 4: startPixel[3] = col;
            case 3: startPixel[2] = col;
            case 2: startPixel[1] = col;
            case 1: startPixel[0] = col;
                break;
            default:
                for (unsigned i = 0; i < rleBuf->runLen; i++)
                    startPixel[i] = col;
            }
            rleBuf++;
        }

        x += glyph.m_width;
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
        for (int i = 0; i < len; i++)
            width += tr->glyphs[(int)text[i]]->m_width;

		return width;
	}
}

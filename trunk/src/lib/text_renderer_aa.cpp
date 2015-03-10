#include "text_renderer_aa.h"

#include "lib/bitmap.h"
#include "lib/common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


// ****************************************************************************
// GlyphAa
// ****************************************************************************

class GlyphAa
{
public:
    int m_minX;
    int m_minY;
    int m_width;
    int m_height;
    int m_pitch;
    unsigned char *m_pixelData; // A 2D array of pixels. Each pixel is a char, representing an alpha value. Number of pixels is m_width * m_height.

	GlyphAa(int minX, int maxX, int minY, int maxY)
	{
        m_minX = minX;
        m_minY = minY;
        if (maxX == 0 || maxY < 0)
        {
            m_width = 0;
            m_height = 0;
            m_pitch = 0;
            m_pixelData = NULL;
        }
        else
        {
            m_width = maxX - minX + 1;
		    m_height = maxY - minY + 1;
            m_pixelData = new unsigned char [m_width * m_height];
        }
	}
};


// ****************************************************************************
// Functions
// ****************************************************************************

static unsigned GetPixelFromBuffer(unsigned *buf, int w, int h, int x, int y)
{
    return buf[(h - y - 1) * w + x];
}


TextRendererAa *CreateTextRendererAa(char const *fontName, int size, int weight)
{
    if (size < 4 || size > 1000 || weight < 1 || weight > 9)
        return NULL;

    TextRendererAa *tr = new TextRendererAa;
    memset(tr, 0, sizeof(TextRendererAa));

    // Get the font from GDI
    HDC winDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    HDC memDC = CreateCompatibleDC(winDC);
    int scaledSize = -MulDiv(size, GetDeviceCaps(memDC, LOGPIXELSY), 72);
    HFONT fontHandle = CreateFont(scaledSize, 0, 0, 0, weight * 100, false, false, false, 
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH,
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

    // Setup stuff needed to render text with GDI
    SetTextColor(memDC, RGB(255,255,255));
    SetBkColor(memDC, 0);
    RECT rect = { 0, 0, tr->maxCharWidth, tr->charHeight };

    // For each ASCII character...
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

        int minX = INT_MAX;
        int maxX = -1;
        int minY = INT_MAX;
        int maxY = -1;

        // Find the dimensions of the glyph
        for (int y = 0; y < tr->charHeight; y++)
        {
            for (int x = 0; x < tr->maxCharWidth; x++)
            {
                unsigned c = GetPixelFromBuffer(gdiPixels, tr->maxCharWidth, tr->charHeight, x, y);
                if ((c & 0xff00) > 0x1000)
                {
                    if (x < minX) 
                        minX = x;
                    if (x > maxX) 
                        maxX = x;
                    if (y < minY) minY = y;
                    if (y > maxY) maxY = y;
                }
            }
        }    
        
        GlyphAa *glyph = new GlyphAa(minX, maxX, minY, maxY);
        unsigned char *pixel = glyph->m_pixelData;
        for (int y = minY; y <= maxY; y++)
		{
            for (int x = minX; x <= maxX; x++)
            {
                unsigned c = GetPixelFromBuffer(gdiPixels, tr->maxCharWidth, tr->charHeight, x, y);
                unsigned char r = (c & 0xff0000) >> 16;
                unsigned char g = (c & 0x00ff00) >> 8;
                unsigned char b = (c & 0x0000ff) >> 0;
//                DebugAssert(r == g && g == b);
                *pixel = 255 - g;
                pixel++;
            }
		}

		tr->glyphs[i] = glyph;
	}

	// Release the GDI resources
    DeleteDC(winDC);
    DeleteDC(memDC);
	DeleteObject(fontHandle);
	DeleteObject(memBmp);
	DeleteObject(prevBmp);

    return tr;
}


static int DrawTextSimpleClipped(TextRendererAa *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int y, char const *text)
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

        GlyphAa *glyph = tr->glyphs[(unsigned char)*text]; 
        unsigned char *glyphPixel = glyph->m_pixelData;

        for (int y = 0; x < glyph->m_height; y++)
        {
            RGBAColour *thisRow = startRow + y * glyph->m_width;
            
            for (int x = 0; x < glyph->m_width; x++)
            {
                thisRow[x].r = *glyphPixel;
                thisRow[x].g = *glyphPixel;
                thisRow[x].b = *glyphPixel;
                glyphPixel++;
            }
        }

        x += glyph->m_width + 25;
        text++;
    }

    return _x - x;
}


int DrawTextSimple(TextRendererAa *tr, RGBAColour col, BitmapRGBA *bmp, int startX, int _y, char const *text)
{
    //     if (x < 0 || y < 0 || (y + tr->charHeight) > (int)bmp->height)
//         return DrawTextSimpleClipped(tr, col, bmp, _x, y, text);
// 
    int currentX = startX;
    while (*text != '\0')
    {
//         if (x + tr->maxCharWidth > (int)bmp->width)
//             break;

        if (*text == 'e')
            text = text;

        GlyphAa *glyph = tr->glyphs[(unsigned char)*text]; 
        unsigned char *glyphPixel = glyph->m_pixelData;
        RGBAColour *destPixel = bmp->pixels + (glyph->m_minY + _y) * bmp->width + currentX;

        for (int y = 0; y < glyph->m_height; y++)
        {
            RGBAColour *thisRow = destPixel + y * bmp->width;

            for (int x = 0; x < glyph->m_width; x++)
            {
                if (*glyphPixel < 255)
                {
                    int alpha = *glyphPixel;
                    int oneMinusAlpha = 255 - alpha;

                    int r = col.r * oneMinusAlpha;
                    int g = col.g * oneMinusAlpha;
                    int b = col.b * oneMinusAlpha;
                    
                    r += thisRow[x].r * alpha;
                    g += thisRow[x].g * alpha;
                    b += thisRow[x].b * alpha;

                    thisRow[x].r = r >> 8;
                    thisRow[x].g = g >> 8;
                    thisRow[x].b = b >> 8;
                }

                glyphPixel++;
            }
        }

        currentX += glyph->m_width + 1;
        text++;
    }

    return currentX - startX;
}


int DrawTextLeft(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimple(tr, c, bmp, x, y, buf);
}


int DrawTextRight(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, x - width, y, buf);
}


int DrawTextCentre(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, x - width/2, y, buf);
}


int GetTextWidth(TextRendererAa *tr, char const *text, int len)
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
            width += tr->glyphs[(int)text[i]]->m_width + 1;

		return width;
	}
}

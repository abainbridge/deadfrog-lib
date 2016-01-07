#include "text_renderer_aa.h"

#include "lib/bitmap.h"
#include "lib/common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


// ****************************************************************************
// GlyphAa
// ****************************************************************************

class GlyphAa
{
public:
    int m_minY;
    int m_width;
    int m_height;
    int m_pitch;
    unsigned char *m_pixelData; // A 2D array of pixels. Each pixel is a char, representing an alpha value. Number of pixels is m_width * m_height.
    signed char m_kerning[255]; // An array containing the kerning offsets need when this glyph is followed by every other possible glyph.

	GlyphAa(int minX, int maxX, int minY, int maxY)
	{
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


// Find the right-hand end of the glyph at the specified scan-line. Return
// the distance from that point to the right-hand edge of the glyph's bounding
// box.
static float GetGapAtRight(GlyphAa *glyph, int y)
{
    //     01234   (glyph->m_width = 5, glyph->m_minY = 1)
    //    +-----+
    //    |*    |   y = 1, gap = 4
    //    |*    |   y = 2, gap = 4
    //    |**** |   y = 3, gap = 1
    //    |*   *|   y = 4, gap = 0
    //    |**** |   y = 5, gap = 1
    //    |     |   y = 6, gap = 5
    //    +-----|

    y -= glyph->m_minY;
    unsigned char *line = glyph->m_pixelData + y * glyph->m_width;

    for (int x = glyph->m_width - 1; x >= 0; x--)
    {
        if (line[x] > 0)
        {
            float gap = glyph->m_width - x;
            gap -= (float)(line[x]) / 255.0;
            return gap;
        }
    }

    return 0.0f;
}


static float GetGapAtLeft(GlyphAa *glyph, int y)
{
    y -= glyph->m_minY;
    unsigned char *line = glyph->m_pixelData + y * glyph->m_width;

    for (int x = 0; x < glyph->m_width; x++)
    {
        if (line[x] > 0)
        {
            float gap = x;
            gap += (255 - line[x]) / 255.0;
            return gap;
        }
    }

    return 0.0f;
}


static unsigned GetKerningDist(GlyphAa *a, GlyphAa *b, int aveCharWidth)
{
    // Calculate how many pixels in the X direction to add after rendering
    // glyph 'a' before glyph 'b'. I call this the kerning distance.
    // Start by assuming that the Kerning distance is just the width of glyph
    // 'a'. We then use a heuristic to determine how big the gap will look
    // when the two glyphs are rendered with this kerning distance. The 
    // heuristic will calculate something like the force of repulsion
    // you'd get if the two glyphs had the same charge. For each row, we 
    // calculate the distance between the two glyphs and say that the force
    // is 1 / sqrt(dist). We sum all the forces and divide by the height of the
    // overlapping regions of the two glyphs. We then iterate this process,
    // gradually reducing the separation until the force reaches a target 
    // amount.

    // Early exit on invisible glyphs
    if (a->m_width == 0)
        return aveCharWidth + 1;

    // Calculate the vertical extent of overlap between the two glyphs.
    int startY = max(a->m_minY, b->m_minY);
    int endY = min(a->m_minY + a->m_height, b->m_minY + b->m_height);

    int maxI = aveCharWidth / 3;
    int minI = -aveCharWidth / 2;
    for (int i = maxI; i > minI; i--)
    {
        float force = 0.0f;
        for (int y = startY; y < endY; y++)
        {   
            float aRight = GetGapAtRight(a, y);
            float bLeft = GetGapAtLeft(b, y);
            float sep = aRight + bLeft + i;
            force += 1.0 / (sep * sep);         // Model the repulsive force using the inverse square law (like coulomb repulsion)
        }

        force /= (float)(endY - startY + 1);
        force *= (aveCharWidth * aveCharWidth) / (30.0f * 30.0f);   // Make force independent of font size
        if (force > 0.01f)
        {
            if (i < 0)
                i /= 2.0;       // Hack alert - The algorithm has a tendency to over do the squeezing up of combinations like "To" and "L'". Assume that any large amount of squeezing up is over done and reduce by half.
            return a->m_width + i;
        }
    }

    return a->m_width + minI/2.0;
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
    tr->aveCharWidth = textMetrics.tmAveCharWidth;
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
                // Only use the green component because it's the middle of the pixel.
                unsigned char g = 255 - ((c & 0x00ff00) >> 8);
                // Test if it is NOT entirely transparent...
                if (g < 255)
                {
                    if (x < minX) 
                        minX = x;
                    if (x > maxX) 
                        maxX = x;
                    if (y < minY) 
                        minY = y;
                    if (y > maxY) 
                        maxY = y;
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
                *pixel = g;
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

    for (int i = 0; i < 255; i++)
    {
        for (int j = 0; j < 255; j++)
        {
            int dist = GetKerningDist(tr->glyphs[i], tr->glyphs[j], tr->aveCharWidth);
            tr->glyphs[i]->m_kerning[j] = dist;
        }
    }

    return tr;
}


static int DrawTextSimpleClippedAa(TextRendererAa *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int y, char const *text)
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


// Alpha is 0 to 255.
static inline void PixelBlend(RGBAColour &d, const RGBAColour s, unsigned char glyphPixel)
{
    const unsigned a     = ((glyphPixel * s.a) >> 8) + 1;

    const unsigned dst_rb = d.c & 0xFF00FF;
    const unsigned dst_g  = d.g;

    const unsigned src_rb = s.c & 0xFF00FF;
    const unsigned src_g  = s.g;

    unsigned d_rb = src_rb - dst_rb;
    unsigned d_g  =  src_g - dst_g;

    d_rb *= a;
    d_g  *= a;
    d_rb >>= 8;
    d_g  >>= 8;

    d.c = (d_rb + dst_rb); // & 0xFF00FF; I'm pretty sure we can never get overflow here and therefore don't need this mask
    d.g = (d_g  + dst_g);
}


int DrawTextSimpleAa(TextRendererAa *tr, RGBAColour col, BitmapRGBA *bmp, int startX, int _y, char const *text)
{
    //     if (x < 0 || y < 0 || (y + tr->charHeight) > (int)bmp->height)
//         return DrawTextSimpleClipped(tr, col, bmp, _x, y, text);
// 
    int currentX = startX;
    while (*text != '\0')
    {
//         if (x + tr->maxCharWidth > (int)bmp->width)
//             break;

        GlyphAa * __restrict glyph = tr->glyphs[(unsigned char)*text]; 
        unsigned char * __restrict glyphPixel = glyph->m_pixelData;
        RGBAColour * __restrict destPixel = bmp->pixels + (glyph->m_minY + _y) * bmp->width + currentX;

        for (int y = 0; y < glyph->m_height; y++)
        {
            RGBAColour * __restrict thisRow = destPixel + y * bmp->width;

            int startX = 0;

            if (glyph->m_width & 1)
            {
                startX = 1;
                PixelBlend(thisRow[0], col, *glyphPixel);
                glyphPixel++;
            }

            for (int x = startX; x < glyph->m_width; x += 2)
            {
                PixelBlend(thisRow[x], col, *glyphPixel);
                glyphPixel++;
                PixelBlend(thisRow[x+1], col, *glyphPixel);
                glyphPixel++;
            }
        }

        text++;

        currentX += glyph->m_kerning[*text];
    }

    return currentX - startX;
}


int DrawTextLeftAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimpleAa(tr, c, bmp, x, y, buf);
}


int DrawTextRightAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidthAa(tr, buf);
    return DrawTextSimpleAa(tr, c, bmp, x - width, y, buf);
}


int DrawTextCentreAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidthAa(tr, buf);
    return DrawTextSimpleAa(tr, c, bmp, x - width/2, y, buf);
}


int GetTextWidthAa(TextRendererAa *tr, char const *text, int len)
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
        {
            GlyphAa *glyph = tr->glyphs[(int)text[i]];
            if (i + 1 < len)
                width += glyph->m_kerning[(int)text[i+1]];
            else
                width += glyph->m_width;
        }

		return width;
	}
}

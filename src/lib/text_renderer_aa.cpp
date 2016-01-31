#include "text_renderer_aa.h"

// Project headers
#include "lib/bitmap.h"
#include "lib/common.h"
#include "lib/master_glyph.h"
#include "lib/sized_glyph.h"
#include "lib/text_renderer_aa_internals.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


// ****************************************************************************
// 
// ****************************************************************************

// class SizedRenderer
// {
// public:
//     SizedGlyph *glyphs[256];
//     int         avgCharWidth;
//     int			maxCharWidth;
//     int			charHeight;	// in pixels
// 
//     SizedRenderer(MasterGlyph *master);
// };



// ****************************************************************************
// Functions
// ****************************************************************************

static unsigned GetKerningDist(MasterGlyph *a, MasterGlyph *b, int avgCharWidth)
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
        return avgCharWidth + 1;

    // Calculate the vertical extent of overlap between the two glyphs.
    int startY = max(a->m_minY, b->m_minY);
    int endY = min(a->m_minY + a->m_height, b->m_minY + b->m_height);

    int maxI = avgCharWidth / 3;
    int minI = -avgCharWidth / 2;
    for (int i = maxI; i > minI; i -= 10)
    {
        float force = 0.0f;
        for (int y = startY; y <= endY; y++)
        {   
            float aRight = a->m_gapsAtRight[y];
            float bLeft = b->m_gapsAtLeft[y];
            float sep = aRight + bLeft + i;
            force += 1.0 / (sep * sep);         // Model the repulsive force using the inverse square law (like coulomb repulsion)
        }

        force /= (float)(endY - startY + 1);
        force *= (avgCharWidth * avgCharWidth) / (30.0f * 30.0f);   // Make force independent of font size
        if (force > 0.01f)
        {
            if (i < 0)
                i /= 2.0;       // Hack alert - The algorithm has a tendency to over do the squeezing up of combinations like "To" and "L'". Assume that any large amount of squeezing up is over done and reduce by half.
            return a->m_width + i;
        }
    }

    return a->m_width + minI/2.0;
}

#include "lib/hi_res_time.h"

TextRendererAa *CreateTextRendererAa(char const *fontName, int weight)
{
    if (weight < 1 || weight > 9)
        return NULL;

    TextRendererAa *tr = new TextRendererAa;
    memset(tr, 0, sizeof(TextRendererAa));

    // Get the font from GDI
    HDC winDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    HDC memDC = CreateCompatibleDC(winDC);
    int scaledSize = -(int)MASTER_GLYPH_RESOLUTION; // -MulDiv(128, GetDeviceCaps(memDC, LOGPIXELSY), 72);
    HFONT fontHandle = CreateFont(scaledSize, 0, 0, 0, weight * 100, false, false, false, 
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        NONANTIALIASED_QUALITY, DEFAULT_PITCH,
        fontName);
    ReleaseAssert(fontHandle != NULL, "Couldn't find Windows font '%s'", fontName);

    // Ask GDI about the font size and fixed-widthness
    SelectObject(memDC, fontHandle); 
    TEXTMETRIC textMetrics;
    GetTextMetrics(memDC, &textMetrics);
//     tr->charHeight = textMetrics.tmHeight;
//     tr->avgCharWidth = textMetrics.tmAveCharWidth;
//     tr->maxCharWidth = textMetrics.tmMaxCharWidth;
    tr->fixedWidth = (textMetrics.tmAveCharWidth == textMetrics.tmMaxCharWidth);

    // Ask GDI about the name of the font
    char nameOfFontWeGot[256];
    GetTextFace(memDC, 256, nameOfFontWeGot);
    ReleaseWarn(strnicmp(nameOfFontWeGot, fontName, 255) == 0,
        "Attempt to load font '%s' failed.\n"
        "'%s' will be used instead.", 
        fontName, nameOfFontWeGot);

    // Create an off-screen GDI bitmap
    BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biBitCount = 1;
    bmpInfo.bmiHeader.biHeight = textMetrics.tmHeight; //tr->charHeight;
    bmpInfo.bmiHeader.biWidth = textMetrics.tmMaxCharWidth; //tr->maxCharWidth;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    unsigned char *gdiPixels = 0;
    HBITMAP memBmp = CreateDIBSection(memDC, (BITMAPINFO *)&bmpInfo, DIB_RGB_COLORS, (void **)&gdiPixels, NULL, 0);
	ReleaseAssert(memBmp != NULL, "In " __FUNCTION__ " CreateDIBSection failed");
    HBITMAP prevBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // Setup stuff needed to render text with GDI
    SetTextColor(memDC, RGB(255,255,255));
    SetBkColor(memDC, 0);
    RECT rect = { 0, 0, textMetrics.tmMaxCharWidth, textMetrics.tmHeight };

    double startTime = GetHighResTime();
    // For each ASCII character...
    for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
//    for (int i = 'A'; i < 'B'; i++)
	{
        char buf[] = {(char)i};

        // Ask GDI to draw the character
		ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, buf, 1, 0);

        // Create our MasterGlyphs from the GDI image
        tr->masterGlyphs[i] = new MasterGlyph;
        tr->masterGlyphs[i]->CreateFromGdiPixels(gdiPixels, textMetrics.tmMaxCharWidth, textMetrics.tmHeight);
	}
    DebugOut("time = %.2fms\n", (GetHighResTime() - startTime) * 1000.0);

	// Release the GDI resources
    DeleteDC(winDC);
    DeleteDC(memDC);
	DeleteObject(fontHandle);
	DeleteObject(memBmp);
	DeleteObject(prevBmp);

    for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
    {
        for (int j = textMetrics.tmFirstChar; j < textMetrics.tmLastChar; j++)
        {
            int dist = GetKerningDist(tr->masterGlyphs[i], tr->masterGlyphs[j], textMetrics.tmAveCharWidth);
            tr->masterGlyphs[i]->m_kerning[j] = dist;
        }
    }

    for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
        tr->masterGlyphs[i]->KerningCalculationComplete();

    return tr;
}


static int DrawTextSimpleClippedAa(TextRendererAa *tr, RGBAColour col, BitmapRGBA *bmp, int _x, int y, int size, char const *text)
{
//     int x = _x;
//     RGBAColour *startRow = bmp->pixels + y * bmp->width;
//     int width = bmp->width;
// 
//     if (y + tr->charHeight < 0 || y > (int)bmp->height)
//         return 0;
// 
//     while (*text != '\0')
//     {
//         if (x + tr->maxCharWidth > (int)bmp->width)
//             break;
// 
//         SizedGlyph *glyph = tr->glyphs[(unsigned char)*text]; 
//         unsigned char *glyphPixel = glyph->m_pixelData;
// 
//         for (int y = 0; x < glyph->m_height; y++)
//         {
//             RGBAColour *thisRow = startRow + y * glyph->m_width;
//             
//             for (int x = 0; x < glyph->m_width; x++)
//             {
//                 thisRow[x].r = *glyphPixel;
//                 thisRow[x].g = *glyphPixel;
//                 thisRow[x].b = *glyphPixel;
//                 glyphPixel++;
//             }
//         }
// 
//         x += glyph->m_width + 25;
//         text++;
//     }
// 
//     return _x - x;
    return 0;
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


DLL_API int DrawTextSimpleAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, int size, char const *text)
{
    if (!tr->size10)
        tr->size10 = new SizedGlyphSet(tr->masterGlyphs, 10);

    //     if (x < 0 || y < 0 || (y + tr->charHeight) > (int)bmp->height)
//         return DrawTextSimpleClipped(tr, col, bmp, _x, y, text);
// 
//     int currentX = startX;
//     while (*text != '\0')
//     {
// //         if (x + tr->maxCharWidth > (int)bmp->width)
// //             break;
// 
//         SizedGlyph * __restrict glyph = tr->glyphs[(unsigned char)*text]; 
//         unsigned char * __restrict glyphPixel = glyph->m_pixelData;
//         RGBAColour * __restrict destPixel = bmp->pixels + (glyph->m_minY + _y) * bmp->width + currentX;
// 
//         for (int y = 0; y < glyph->m_height; y++)
//         {
//             RGBAColour * __restrict thisRow = destPixel + y * bmp->width;
// 
//             int startX = 0;
// 
//             if (glyph->m_width & 1)
//             {
//                 startX = 1;
//                 PixelBlend(thisRow[0], col, *glyphPixel);
//                 glyphPixel++;
//             }
// 
//             for (int x = startX; x < glyph->m_width; x += 2)
//             {
//                 PixelBlend(thisRow[x], col, *glyphPixel);
//                 glyphPixel++;
//                 PixelBlend(thisRow[x+1], col, *glyphPixel);
//                 glyphPixel++;
//             }
//         }
// 
//         text++;
// 
//         currentX += glyph->m_kerning[*text];
//     }
// 
//     return currentX - startX;
    return 0;
}


int DrawTextLeftAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, int size, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimpleAa(tr, c, bmp, x, y, size, buf);
}


int DrawTextRightAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, int size, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidthAa(tr, buf);
    return DrawTextSimpleAa(tr, c, bmp, x - width, y, size, buf);
}


int DrawTextCentreAa(TextRendererAa *tr, RGBAColour c, BitmapRGBA *bmp, int x, int y, int size, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidthAa(tr, buf);
    return DrawTextSimpleAa(tr, c, bmp, x - width/2, y, size, buf);
}


int GetTextWidthAa(TextRendererAa *tr, char const *text, int len)
{
    return 0;
// 	len = min((int)strlen(text), len);
// 	if (tr->fixedWidth)
// 	{
// 		return len * tr->maxCharWidth;
// 	}
// 	else
// 	{
//         int width = 0;
//         for (int i = 0; i < len; i++)
//         {
//             SizedGlyph *glyph = tr->glyphs[(int)text[i]];
//             if (i + 1 < len)
//                 width += glyph->m_kerning[(int)text[i+1]];
//             else
//                 width += glyph->m_width;
//         }
// 
// 		return width;
// 	}
}

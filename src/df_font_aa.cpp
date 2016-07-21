#include "df_font_aa.h"

// Project headers
#include "df_bitmap.h"
#include "df_common.h"
#include "df_master_glyph.h"
#include "df_sized_glyph.h"
#include "df_font_aa_internals.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


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
    int yOverlap = min(a->m_height, b->m_height);

    int maxI = avgCharWidth / 3;
    int minI = -avgCharWidth / 2;
    for (int i = maxI; i > minI; i -= 5)
    {
        float force = 0.0f;
        for (int y = 0; y < yOverlap; y++)
        {   
            float aRight = a->m_gapsAtRight[y];
            float bLeft = b->m_gapsAtLeft[y];
            float sep = aRight + bLeft + i;
            force += 1.0 / (sep * sep);         // Model the repulsive force using the inverse square law (like coulomb repulsion)
        }

        force /= (float)(yOverlap + 1);
        force *= (avgCharWidth * avgCharWidth) / (3.0f * 3.0f);   // Make force independent of font size
        if (force > 1.0f)
        {
            if (i < 0)
                i /= 2.0;       // Hack alert - The algorithm has a tendency to over do the squeezing up of combinations like "To" and "L'". Assume that any large amount of squeezing up is over done and reduce by half.
            return a->m_width + i;
        }
    }

    return a->m_width + minI/2.0;
}


DfFontAa *CreateFontAa(char const *fontName, int weight)
{
    if (weight < 1 || weight > 9)
        return NULL;

    DfFontAa *tr = new DfFontAa;
    memset(tr, 0, sizeof(DfFontAa));

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
    tr->fixedWidth = !(textMetrics.tmPitchAndFamily & TMPF_FIXED_PITCH);

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

    // For each ASCII character...
    for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
	{
        char buf[] = {(char)i};

        // Ask GDI to draw the character
		ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, buf, 1, 0);

        // Create our MasterGlyphs from the GDI image
        tr->masterGlyphs[i] = new MasterGlyph;
        tr->masterGlyphs[i]->CreateFromGdiPixels(gdiPixels, textMetrics.tmMaxCharWidth, textMetrics.tmHeight, tr->fixedWidth);
	}

	// Release the GDI resources
    DeleteDC(winDC);
    DeleteDC(memDC);
	DeleteObject(fontHandle);
	DeleteObject(memBmp);
	DeleteObject(prevBmp);

    if (tr->fixedWidth)
    {
        for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
            for (int j = textMetrics.tmFirstChar; j < textMetrics.tmLastChar; j++)
                tr->masterGlyphs[i]->m_kerning[j] = textMetrics.tmAveCharWidth;
    }
    else
    {
        // Create look-up tables to accelerate kerning calculations
        for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
        {
            for (int j = textMetrics.tmFirstChar; j < textMetrics.tmLastChar; j++)
            {
                int dist = GetKerningDist(tr->masterGlyphs[i], tr->masterGlyphs[j], textMetrics.tmAveCharWidth);
                tr->masterGlyphs[i]->m_kerning[j] = dist;
            }
        }

        // Determine the kerning distances for all possible pairs of glyphs
        for (int i = textMetrics.tmFirstChar; i < textMetrics.tmLastChar; i++)
            tr->masterGlyphs[i]->KerningCalculationComplete();
    }

    return tr;
}


// Alpha is 0 to 255.
static inline void PixelBlend(DfColour &d, const DfColour s, unsigned char glyphPixel)
{
    const unsigned a = ((glyphPixel * s.a) >> 8) + 1;

    const unsigned dst_rb = d.c & 0xFF00FF;
    const unsigned dst_g = d.g;

    const unsigned src_rb = s.c & 0xFF00FF;
    const unsigned src_g = s.g;

    unsigned d_rb = src_rb - dst_rb;
    unsigned d_g = src_g - dst_g;

    d_rb *= a;
    d_g *= a;
    d_rb >>= 8;
    d_g >>= 8;

    d.c = (d_rb + dst_rb); // & 0xFF00FF; I'm pretty sure we can never get overflow here and therefore don't need this mask
    d.g = (d_g + dst_g);
}


static void DrawClippedGlyph(SizedGlyph *glyph, DfBitmap *bmp, int x0, int y0, DfColour col)
{
    for (int gy = 0; gy < glyph->m_height; gy++)
    {
        int y = y0 + gy;
        if (y < 0 || y >= (int)bmp->height)
            continue;

        unsigned char *glyphRow = glyph->m_pixelData + gy * glyph->m_width;
        DfColour *bmpRow = bmp->pixels + y * bmp->width;

        for (int gx = 0; gx < glyph->m_width; gx++)
        {
            int x = x0 + gx;
            if (x < 0 || x >= (int)bmp->width)
                continue;

            PixelBlend(bmpRow[x], col, glyphRow[gx]);
        }
    }
}


static SizedGlyphSet *GetSizedGlyphSet(DfFontAa *tr, int size)
{
    if (!tr->sizedGlyphSets[size])
        tr->sizedGlyphSets[size] = new SizedGlyphSet(tr->masterGlyphs, size);
    SizedGlyphSet *sizedGlyphSet = tr->sizedGlyphSets[size];
    return sizedGlyphSet;
}


static int DrawTextSimpleClippedAa(DfFontAa *tr, DfColour col, DfBitmap *bmp, int x, int y, int size, char const *text)
{
    if (x >= (int)bmp->width || y + size < 0 || y >= (int)bmp->height)
        return 0;

    SizedGlyphSet *sizedGlyphSet = GetSizedGlyphSet(tr, size);
    int startX = x;

    while (*text != '\0')
    {
        SizedGlyph *glyph = sizedGlyphSet->m_sizedGlyphs[(unsigned char)*text];
        if (!glyph)
            x += size;
        else
            DrawClippedGlyph(glyph, bmp, x, y, col);

        x += glyph->m_kerning[*(text + 1)];
        text++;
    }

    return startX - x;
}


DLL_API int DrawTextSimpleAa(DfFontAa *tr, DfColour col, DfBitmap *bmp, int x, int y, int size, char const *text)
{
    if (size < 1 || size > MAX_FONT_SIZE)
        return -1;

    if (x < 0 || y < 0 || (y + size) > (int)bmp->height)
        return DrawTextSimpleClippedAa(tr, col, bmp, x, y, size, text);

    SizedGlyphSet *sizedGlyphSet = GetSizedGlyphSet(tr, size);

    int startX = x;
    int currentX = x;
    while (*text != '\0')
    {
        SizedGlyph * __restrict glyph = sizedGlyphSet->m_sizedGlyphs[(unsigned char)*text];

        if (!glyph)
        {   
            currentX += size;
        }
        else
        {
            if (x + glyph->m_width >= (int)bmp->width) {
                DrawClippedGlyph(glyph, bmp, x, y, col);
                break;
            }

            unsigned char * __restrict glyphPixel = glyph->m_pixelData;
            DfColour * __restrict thisRow = bmp->pixels + y * bmp->width + currentX;
            DfColour * __restrict lastRow = thisRow + glyph->m_height * bmp->width;
            while (thisRow < lastRow)
            {
                for (int x = 0; x < glyph->m_width; x++)
                {
                    PixelBlend(thisRow[x], col, *glyphPixel);
                    glyphPixel++;
                }

                thisRow += bmp->width;
            }

            currentX += glyph->m_kerning[*(text + 1)];
        }

        text++;
    }

    return currentX - startX;
}


int DrawTextLeftAa(DfFontAa *tr, DfColour c, DfBitmap *bmp, int x, int y, int size, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimpleAa(tr, c, bmp, x, y, size, buf);
}


int DrawTextRightAa(DfFontAa *tr, DfColour c, DfBitmap *bmp, int x, int y, int size, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidthAa(tr, buf, size);
    return DrawTextSimpleAa(tr, c, bmp, x - width, y, size, buf);
}


int DrawTextCentreAa(DfFontAa *tr, DfColour c, DfBitmap *bmp, int x, int y, int size, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidthAa(tr, buf, size);
    return DrawTextSimpleAa(tr, c, bmp, x - width/2, y, size, buf);
}


int GetTextWidthAa(DfFontAa *tr, char const *text, int size, int len)
{
    SizedGlyphSet *sizedGlyphSet = GetSizedGlyphSet(tr, size);

    len = min((int)strlen(text), len);
	if (tr->fixedWidth)
	{
        SizedGlyph *glyphA = sizedGlyphSet->m_sizedGlyphs['a'];
        return len * glyphA->m_kerning['a'];
	}
	else
	{
        int width = 0;
        for (int i = 0; i < len; i++)
        {
            SizedGlyph *glyph = sizedGlyphSet->m_sizedGlyphs[(unsigned char)text[i]];
            if (i + 1 < len)
                width += glyph->m_kerning[(int)text[i+1]];
            else
                width += glyph->m_width;
        }

		return width;
	}
}

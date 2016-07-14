// Own header
#include "df_master_glyph.h"

// Project headers
#include "df_font_aa_internals.h"

// Standard headers
#include <algorithm>
#include <math.h>
#include <limits.h>


static const unsigned NUM_GAPS = 20;    // Number of samples (in the Y dimension) to use to calculate the kerning distance.


static bool GetPixelFromGdiBuffer(unsigned char *buf, int byteW, int h, int x, int y)
{
    y = h - y - 1;
    unsigned char *line = buf + y * byteW;
    unsigned char pixelByte = line[x / 8];
    unsigned char shiftedPixelByte = pixelByte >> ((7 - x) & 0x7);
    return shiftedPixelByte & 1;
}


void MasterGlyph::CreateFromGdiPixels(unsigned char *gdiPixels, int gdiPixelsWidth, int gdiPixelsHeight, bool fixedWidth)
{
    unsigned gdiByteWidth = (int)ceil(gdiPixelsWidth / 32.0) * 4;

    int minX = INT_MAX;
    int maxX = -1;
    int maxY = -1;

    // Find the dimensions of the glyph
    for (int y = 0; y < gdiPixelsHeight; y++)
    {
        for (int x = 0; x < gdiPixelsWidth; x++)
        {
            bool p = GetPixelFromGdiBuffer(gdiPixels, gdiByteWidth, gdiPixelsHeight, x, y);
            if (p)
            {
                minX = std::min(x, minX);
                maxX = std::max(x, maxX);
                maxY = std::max(y, maxY);
            }
        }
    }    

    if (fixedWidth)
        minX = 0;

    if (maxX == 0 || maxY < 0)
    {
        m_width = 0;
        m_widthBytes = 0;
        m_height = 0;
        m_pixelData = NULL;
        m_gapsAtLeft = m_gapsAtRight = NULL;
    }
    else
    {
        m_width = maxX - minX + 1;
        m_widthBytes = (int)ceil(m_width / 8.0);
        m_height = maxY + 1;
        m_pixelData = new unsigned char [m_widthBytes * m_height];
        memset(m_pixelData, 0, m_widthBytes * m_height);

        // Copy the pixel data from the GDI bitmap into this glyph's own store.
        for (int y = 0; y < m_height; y++)
        {
            for (int x = 0; x < m_width; x++)
            {
                if (GetPixelFromGdiBuffer(gdiPixels, gdiByteWidth, gdiPixelsHeight, x + minX, y))
                {
                    PutPix(x, y);
                }
            }
        }

        m_gapsAtLeft = new int[NUM_GAPS];
        m_gapsAtRight = new int[NUM_GAPS];
        for (unsigned i = 0; i < NUM_GAPS; i++)
        {
            CalcGapAtLeft(i);
            CalcGapAtRight(i);
        }
    }
}


// Find the right-hand end of the glyph at the specified scan-line. Return
// the distance from that point to the right-hand edge of the glyph's bounding
// box.
void MasterGlyph::CalcGapAtRight(int i)
{
    //     01234    (m_width = 5)
    //    +-----+
    //    |*    |   y = 1, gap = 4
    //    |*    |   y = 2, gap = 4
    //    |**** |   y = 3, gap = 1
    //    |*   *|   y = 4, gap = 0
    //    |**** |   y = 5, gap = 1
    //    |     |   y = 6, gap = 5
    //    +-----|

    int y = (int)((double)MASTER_GLYPH_RESOLUTION * (double)i / (double)NUM_GAPS);

    if (y < 0 || y >= m_height)
    {
        m_gapsAtRight[i] = m_width;
        return;
    }

    for (int x = m_width - 1; x >= 0; x--)
    {
        if (GetPix(x, y))
        {
            m_gapsAtRight[i] = m_width - x;
            return;
        }
    }

    m_gapsAtRight[i] = m_width; // TODO, should this be zero?
}


void MasterGlyph::CalcGapAtLeft(int i)
{
    int y = (int)((double)MASTER_GLYPH_RESOLUTION * (double)i / (double)NUM_GAPS);

    if (y < 0 || y >= m_height)
    {
        m_gapsAtLeft[i] = m_width;
        return;
    }

    for (int x = 0; x < m_width; x++)
    {
        if (GetPix(x, y))
        {
            m_gapsAtLeft[i] = x;
            return;
        }
    }

    m_gapsAtLeft[i] = m_width; // TODO, should this be zero?
}


void MasterGlyph::KerningCalculationComplete()
{
    delete [] m_gapsAtLeft;
    delete [] m_gapsAtRight;
    m_gapsAtLeft = m_gapsAtRight = NULL;
}


bool MasterGlyph::GetPix(unsigned x, unsigned y)
{
    unsigned char *line = m_pixelData + y * m_widthBytes;
    unsigned char pixelByte = line[x / 8];
    unsigned char shiftedPixelByte = pixelByte >> (x & 0x7);
    return shiftedPixelByte & 1;
}


void MasterGlyph::PutPix(unsigned x, unsigned y)
{
    unsigned char *line = m_pixelData + y * m_widthBytes;
    unsigned char pixelMask = 1 << (x & 0x7);
    line[x / 8] |= pixelMask;
}

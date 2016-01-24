// Own header
#include "master_glyph.h"

// Standard headers
#include <algorithm>
#include <math.h>
#include <limits.h>


static const unsigned NUM_GAPS = 20;


static bool GetPixelFromGdiBuffer(unsigned char *buf, int byteW, int h, int x, int y)
{
    y = h - y - 1;
    unsigned char *line = buf + y * byteW;
    unsigned char pixelByte = line[x / 8];
    unsigned char shiftedPixelByte = pixelByte >> ((7 - x) & 0x7);
    return shiftedPixelByte & 1;
}


#include "lib/bitmap.h"
#include "lib/window_manager.h"

void MasterGlyph::CreateFromGdiPixels(unsigned char *gdiPixels, int gdiPixelsWidth, int gdiPixelsHeight)
{
    unsigned gdiByteWidth = (int)ceil(gdiPixelsWidth / 32.0) * 4;

    int minX = INT_MAX;
    int maxX = -1;
    int minY = INT_MAX;
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
                minY = std::min(y, minY);
                maxY = std::max(y, maxY);
            }
        }
    }    

    m_minY = minY;
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
        m_height = maxY - minY + 1;
        m_pixelData = new unsigned char [m_widthBytes * m_height];
        memset(m_pixelData, 0, m_widthBytes * m_height);

        // Copy the pixel data from the GDI bitmap into this glyph's own store.
        for (int y = 0; y < m_height; y++)
        {
            for (int x = 0; x < m_width; x++)
            {
                if (GetPixelFromGdiBuffer(gdiPixels, gdiByteWidth, gdiPixelsHeight, x + minX, y + minY))
                {
//                    ::PutPix(g_window->bmp, x+500, y, Colour(255,0,255));
                    PutPix(x, y);
                }
            }
        }

        m_gapsAtLeft = new float[NUM_GAPS];
        m_gapsAtRight = new float[NUM_GAPS];
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
    //     01234   (m_width = 5, m_minY = 1)
    //    +-----+
    //    |*    |   y = 1, gap = 4
    //    |*    |   y = 2, gap = 4
    //    |**** |   y = 3, gap = 1
    //    |*   *|   y = 4, gap = 0
    //    |**** |   y = 5, gap = 1
    //    |     |   y = 6, gap = 5
    //    +-----|

    int y = (int)((double)i / (double)NUM_GAPS);

    if (y < m_minY || y >= (m_minY + m_height))
    {
        m_gapsAtRight[i] = m_width;
        return;
    }

    int localY = y - m_minY;
    unsigned char *line = m_pixelData + localY * m_width;

    for (int x = m_width - 1; x >= 0; x--)
    {
        if (line[x] > 0)
        {
            float gap = m_width - x;
            gap -= (float)(line[x]) / 255.0;
            m_gapsAtRight[i] = gap;
            return;
        }
    }

    m_gapsAtRight[i] = 0.0f;
}


void MasterGlyph::CalcGapAtLeft(int i)
{
    int y = (int)((double)i / (double)NUM_GAPS);

    if (y < m_minY || y >= (m_minY + m_height))
    {
        m_gapsAtLeft[i] = m_width;
        return;
    }

    int localY = y - m_minY;
    unsigned char *line = m_pixelData + localY * m_width;

    for (int x = 0; x < m_width; x++)
    {
        if (line[x] > 0)
        {
            float gap = x;
            gap += (255 - line[x]) / 255.0;
            m_gapsAtLeft[i] = gap;
            return;
        }
    }

    m_gapsAtLeft[i] = 0.0f;
}


void MasterGlyph::FreeKerningTables()
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

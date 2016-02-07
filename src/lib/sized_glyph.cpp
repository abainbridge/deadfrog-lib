// Own header
#include "sized_glyph.h"

// Project headers
#include "master_glyph.h"
#include "text_renderer_aa_internals.h"

// Standard headers
#include <algorithm>
#include <stdlib.h>


// ***************************************************************************
// SizedGlyph
// ***************************************************************************

void SizedGlyph::PutPix(unsigned x, unsigned y, unsigned char val)
{
    unsigned char *line = m_pixelData + y * m_width;
    line[x] = val;
}


SizedGlyph::SizedGlyph(MasterGlyph *mg, unsigned output_size)
{
    // The downsampling code below is based on code written by Ryan Geiss. 
    // (http://www.geisswerks.com/ryan/FAQS/resize.html)

    double scale_factor = (double)output_size / (double)MASTER_GLYPH_RESOLUTION;
    m_height = round(mg->m_height * scale_factor);
    m_width = round(mg->m_width * scale_factor);
    m_pixelData = new unsigned char[m_width * m_height];

    int w1 = mg->m_width;
    int h1 = mg->m_height;

    int w2 = m_width;
    int h2 = m_height;

    float fh = 256*h1 / (float)h2;
    float fw = 256*w1 / (float)w2;

    // FOR EVERY OUTPUT PIXEL
    for (int y2 = 0; y2 < h2; y2++)
    {   
        // find the y-range of input pixels that will contribute:
        int y1a = (int)((y2  )*fh); 
        int y1b = (int)((y2+1)*fh); 
        y1b = std::min(y1b, 256*h1 - 1);
        int y1c = y1a >> 8;
        int y1d = y1b >> 8;
        y1a &= 0xff;
        y1b &= 0xff;

        for (int x2 = 0; x2 < w2; x2++)
        {
            // find the x-range of input pixels that will contribute:
            int x1a = (int)((x2)*fw);
            int x1b = (int)((x2 + 1)*fw);
            x1b = std::min(x1b, 256 * w1 - 1);
            int x1c = x1a >> 8;
            int x1d = x1b >> 8;
            x1a &= 0xff;
            x1b &= 0xff;

            // ADD UP ALL INPUT PIXELS CONTRIBUTING TO THIS OUTPUT PIXEL:
            unsigned int accumulatedBrightness = 0;
            for (int y = y1c; y <= y1d; y++)
            {
                unsigned int weight_y = 256;
                if (y1c != y1d) 
                {
                    if (y == y1c)
                        weight_y = 256 - y1a;
                    else if (y == y1d)
                        weight_y = y1b;
                }

                unsigned char *line = mg->m_pixelData + y * mg->m_widthBytes;
                unsigned accumulatedBrightnessX = 0;

                unsigned int weight_x = 256 - x1a;
                unsigned char pixelByte = line[x1c >> 3];
                unsigned char shiftedPixelByte = pixelByte >> (x1c & 0x7);
                accumulatedBrightnessX += (shiftedPixelByte & 1) * weight_x;

                for (int x = x1c + 1; x < x1d; x++)
                {
                    pixelByte = line[x >> 3];
                    shiftedPixelByte = pixelByte >> (x & 0x7);                   
                    accumulatedBrightnessX += (shiftedPixelByte & 1) << 8;
                }

                pixelByte = line[x1d >> 3];
                shiftedPixelByte = pixelByte >> (x1d & 0x7);
                accumulatedBrightnessX += (shiftedPixelByte & 1) * x1b;

                accumulatedBrightness += accumulatedBrightnessX * weight_y;
            }

            int xSize = (256 - x1a) + (256 * (x1d - x1c - 1)) + (x1b);
            int ySize = (256 - y1a) + (256 * (y1d - y1c - 1)) + (y1b);
            int accumulatedArea = xSize * ySize;

            // write results
            unsigned int c = accumulatedBrightness / (accumulatedArea >> 8);
            c = std::min(c, (unsigned)255);
            PutPix(x2, y2, c);
        }
    }

    for (unsigned i = 0; i < 255; i++)
        m_kerning[i] = round(mg->m_kerning[i] * scale_factor) + 1;
}


// ***************************************************************************
// SizedGlyphSet
// ***************************************************************************

SizedGlyphSet::SizedGlyphSet(MasterGlyph *masterGlyphSet[256], int size)
{
    for (int i = 0; i < 256; i++)
    {
        MasterGlyph *mg = masterGlyphSet[i];
        if (mg)
        {
            m_sizedGlyphs[i] = new SizedGlyph(mg, size);
        }
        else
        {
            m_sizedGlyphs[i] = NULL;
        }
    }
}

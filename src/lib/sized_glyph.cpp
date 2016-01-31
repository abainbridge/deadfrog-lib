// Own header
#include "sized_glyph.h"

// Project headers
#include "master_glyph.h"
#include "text_renderer_aa_internals.h"

// Standard headers
#include <algorithm>
#include "text_renderer_aa_internals.h"


// ***************************************************************************
// SizedGlyph
// ***************************************************************************

void SizedGlyph::PutPix(unsigned x, unsigned y, unsigned char val)
{
    unsigned char *line = m_pixelData + (y - m_minY) * m_width;
    line[x] = val;
}


SizedGlyph::SizedGlyph(MasterGlyph *mg, unsigned output_size)
{
    double scale_factor = (double)output_size / (double)MASTER_GLYPH_RESOLUTION;
    m_minY = floor(mg->m_minY * scale_factor);
    int maxY = ceil((mg->m_minY + mg->m_height) * scale_factor);
    m_height = maxY - m_minY + 1;
    m_width = ceil(mg->m_width * scale_factor);
    m_pixelData = new unsigned char[m_width * m_height];

    int w1 = mg->m_width;
    int h1 = mg->m_height;

    int w2 = int((double)w1 * scale_factor);
    int h2 = int((double)h1 * scale_factor);

    float fh = 256*h1 / (float)h2;
    float fw = 256*w1 / (float)w2;

    // cache x1a, x1b for all the columns:
    int g_px1ab[128 * 2 * 2];   // Don't understand the need for the second *2
    for (int x2 = 0; x2 < w2; x2++)
    {
        // find the x-range of input pixels that will contribute:
        int x1a = (int)((x2  )*fw); 
        int x1b = (int)((x2+1)*fw); 
        x1b = std::min(x1b, 256*w1 - 1);
        g_px1ab[x2*2+0] = x1a;
        g_px1ab[x2*2+1] = x1b;
    }

    // FOR EVERY OUTPUT PIXEL
    for (int y2 = 0; y2 < h2; y2++)
    {   
        // find the y-range of input pixels that will contribute:
        int y1a = (int)((y2  )*fh); 
        int y1b = (int)((y2+1)*fh); 
        y1b = std::min(y1b, 256*h1 - 1);
        int y1c = y1a >> 8;
        int y1d = y1b >> 8;

        for (int x2 = 0; x2 < w2; x2++)
        {
            // find the x-range of input pixels that will contribute:
            int x1a = g_px1ab[x2*2+0];    // (computed earlier)
            int x1b = g_px1ab[x2*2+1];    // (computed earlier)
            int x1c = x1a >> 8;
            int x1d = x1b >> 8;

            // ADD UP ALL INPUT PIXELS CONTRIBUTING TO THIS OUTPUT PIXEL:
            unsigned int r = 0;
            unsigned int a = 0;
            for (int y = y1c; y <= y1d; y++)
            {
                unsigned int weight_y = 256;
                if (y1c != y1d) 
                {
                    if (y == y1c)
                        weight_y = 256 - (y1a & 0xFF);
                    else if (y == y1d)
                        weight_y = (y1b & 0xFF);
                }

                for (int x = x1c; x <= x1d; x++)
                {
                    unsigned int weight_x = 256;
                    if (x1c != x1d) 
                    {
                        if (x == x1c)
                            weight_x = 256 - (x1a & 0xFF);
                        else if (x == x1d)
                            weight_x = (x1b & 0xFF);
                    }

                    unsigned int c = 0;//*dsrc2++;//dsrc[y*w1 + x];
                    if (mg->GetPix(x, y))
                        c = 1;
                    unsigned int r_src = c;
                    unsigned int w = weight_x * weight_y;
                    r += r_src * w;
                    a += w;
                }
            }

            // write results
            unsigned int c = r/a;
            PutPix(x2, y2, c);
            //*ddest++ = c;//ddest[y2*w2 + x2] = c;
        }
    }

    for (unsigned i = 0; i < 255; i++)
        m_kerning[i] = output_size;
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
            m_sizedGlyphs[i] = 0;
        }
    }
}

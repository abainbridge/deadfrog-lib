// Own header
#include "df_sized_glyph.h"

// Project headers
#include "df_master_glyph.h"
#include "df_font_aa_internals.h"

// Standard headers
#include <algorithm>
#include <math.h>
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

    // There's some dodgy looking maths below. The problem is that we can't just
    // figure out the output size of this glyph from the MasterGlyph size multiplied
    // by the scale factor. If we do that, we introduce a different amount of 
    // rounding error for each glyph. For example, say the MasterGlyph for an 'x'
    // is 50 pixels high and a 'g' is 65 pixels high, output_size is 10 and
    // MASTER_GLYPH_RESOLUTION is 128:
    //   scale_factor = 0.0781
    //   m_height for 'x' is ceil(50 * 0.0781) = ceil(3.906) = 4
    //   m_height for 'g' is ceil(65 * 0.0781) = ceil(5.078) = 6
    //   The amount we've scaled 'x' by is 4/50 = 0.0800
    //   The amount we've scaled 'g' by is 6/65 = 0.0923
    // That's a 15% difference! If we did it this way, it would be easy to see
    // the different amounts of scaling in adjacent glyphs - mainly because we
    // expect all the glyphs to sit evenly on a line, the different scales
    // makes them different heights and therefore they don't all sit on that
    // line.
    //
    // So, instead, we pretend that all MasterGlyphs are MASTER_GLYPH_RESOLUTION
    // in height. And therefore in the bitmap scaling code below, we pretend
    // that the height of the output glyph is output_size. We have to be careful
    // not to get out-of-bounds errors when reading from the MasterGlyph, but 
    // that isn't hard to deal with.
    //
    // We don't currently worry about the same problem for the horizontal
    // dimension, because the eye is much less sensitive to errors there.

    m_height = ceil(mg->m_height * scale_factor);
    m_width = round(mg->m_width * scale_factor);
    m_pixelData = new unsigned char[m_width * m_height];

    int w1 = mg->m_width;
    int h1 = MASTER_GLYPH_RESOLUTION;   // Was mg->m_height. See note above.

    int w2 = m_width;
    int h2 = output_size;               // Was m_height. See note above.

    float fh = 256*h1 / (float)h2;
    float fw = 256*w1 / (float)w2;

    // FOR EVERY OUTPUT PIXEL
    for (int y2 = 0; y2 < m_height; y2++)
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
                
                if (y >= mg->m_height)
                    continue;

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

            // Calculate brightness for the target pixel from the accumulated result.
            double linearBrightness = accumulatedBrightness / (double)(accumulatedArea >> 8);
            
            // Gamma correct it.
            const double gamma = 1.0 / 2.2;
            const double gammaScale = 255.0 / pow(255.0, gamma);
            unsigned c = pow(linearBrightness, gamma) * gammaScale;
            
            // Clamp it to range.
            c = std::min(c, (unsigned)255);

            // Write it.
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

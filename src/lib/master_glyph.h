#pragma once


// Coordinate space:
//
// -------------
//    *            y = 0             For the 'A' glyph, m_minY = 0
//   * *           y = 1
//  *   *          y = 2
//  *****  ***     y = 3             For the 'g' glyph, m_minY = 3
//  *   * *  *     y = 4
//  *   *  ***     y = 5
//           *     y = 6
//         **      y = 7 = m_height
// -------------


class MasterGlyph
{
public:
    int m_width;                // Width of glyph in pixels
    int m_widthBytes;           // Width of glyph in bytes (differs from m_width/8 due to padding)
    int m_height;
    unsigned char *m_pixelData; // A 2D array of pixels. Each pixel is 1 bit. Number of pixels is m_width * m_height.
    signed char m_kerning[255]; // An array containing the kerning offsets need when this glyph is followed by every other possible glyph.
    int *m_gapsAtLeft;          // Look up table used to speed up kerning calculations. Freed once font creation is complete.
    int *m_gapsAtRight;         // As above

private:
    void PutPix(unsigned x, unsigned y);
    void CalcGapAtLeft(int i);
    void CalcGapAtRight(int i);

public:
    void CreateFromGdiPixels(unsigned char *gdiPixels, int gdiPixelsWidth, int gdiPixelsHeight, bool fixedWidth);
    void KerningCalculationComplete();

    bool GetPix(unsigned x, unsigned y);
};

// TODO. Store an offset that describes the size of the gap to leave at the left 
// of the glyph. This is only useful for fixed-pitch fonts. It will be used to
// when the glyph is narrower than the pitch in order to centralize the glyph
// in its area. At the moment we solve the problem by encoding the gap at the
// left as part of the glyph, which is wasteful of memory and cycles when 
// rendering.
// TODO. Store pixel data run-length encoded.

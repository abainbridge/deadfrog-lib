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
                                // No need for m_minX because it would always be zero.
    int m_minY;
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
    void CreateFromGdiPixels(unsigned char *gdiPixels, int gdiPixelsWidth, int gdiPixelsHeight);
    void KerningCalculationComplete();

    bool GetPix(unsigned x, unsigned y);
};

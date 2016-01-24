#pragma once

class SizedGlyph
{
public:
    int m_minY;
    int m_width;
    int m_height;
    unsigned char *m_pixelData; // A 2D array of pixels. Each pixel is a char, representing an alpha value. Number of pixels is m_width * m_height.
    signed char m_kerning[255]; // An array containing the kerning offsets need when this glyph is followed by every other possible glyph.

	SizedGlyph(unsigned *gdiPixels, int gdiPixelsWidth, int gdiPixelsHeight);
};

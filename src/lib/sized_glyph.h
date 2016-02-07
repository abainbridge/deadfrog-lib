#pragma once


class MasterGlyph;


class SizedGlyph
{
public:
    int m_width;
    int m_height;
    unsigned char *m_pixelData; // A 2D array of pixels. Each pixel is a char, representing an alpha value. Number of pixels is m_width * m_height.
    signed char m_kerning[255]; // An array containing the kerning offsets need when this glyph is followed by every other possible glyph.

    void PutPix(unsigned x, unsigned y, unsigned char val);

	SizedGlyph(MasterGlyph *mg, unsigned size);
};


class SizedGlyphSet
{
public:
    SizedGlyph *m_sizedGlyphs[256]; // Blank or missing glyphs will be represented by a NULL pointer

    SizedGlyphSet(MasterGlyph *masterGlyphSet[256], int size);
};

// TODO: Generate the glyphs on demand, instead of in SizedGlyphSet's constructor. 
//       This will require that blank or missing glyphs are no longer represented
//       by a NULL pointer.

#include "df_font.h"

#include "df_bitmap.h"
#include "df_common.h"

#if _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include "df_bmp.h"
#endif

#include <limits.h>
#include <memory.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


// ****************************************************************************
// Glyph
// ****************************************************************************

typedef struct
{
    unsigned startX;
    unsigned startY;
    unsigned runLen;
} EncodedRun;


class Glyph
{
public:
    unsigned m_width;
    int m_numRuns;
    EncodedRun *m_pixelRuns;

    Glyph(int w)
    {
        m_width = w;
    }
};


// ****************************************************************************
// Global variables
// ****************************************************************************

DfFont *g_defaultFont = NULL;


// ****************************************************************************
// Public Functions
// ****************************************************************************

static bool GetPixelFromBuffer(unsigned *buf, int w, int h, int x, int y)
{
    return buf[(h - y - 1) * w + x] > 0;
}


static int GetPixel(char *tmpBitmap, int bmpWidth, int x, int y) {
    return tmpBitmap[y * bmpWidth + x];
}


struct MemBuf {
    unsigned char *data;
    int len;
    int currentPos;
    int currentByte;
    int hiNibbleNext;

    MemBuf(unsigned *_data, int _len) {
        data = (unsigned char *)_data;
        len = _len;
        currentPos = 0;
        currentByte = 0;
        hiNibbleNext = 0;
    }

    int ReadByte() {
        ReleaseAssert(currentPos < len, "Past end");
        currentByte = data[currentPos];
        currentPos++;
        return currentByte;
    }

    int ReadNibble() {
        if (hiNibbleNext) {
            hiNibbleNext = 0;
            return currentByte >> 4;
        }

        ReadByte();
        hiNibbleNext = 1;
        return currentByte & 0xf;
    }
};


DfFont *LoadFontFromMemory(void *_buf, int bufLen)
{
    ReleaseAssert(bufLen > 10, "Font file too small");
    MemBuf buf((unsigned int *)_buf, bufLen);

    DfFont *tr = new DfFont;
    memset(tr, 0, sizeof(DfFont));

    tr->maxCharWidth = buf.ReadByte();
    tr->charHeight = buf.ReadByte();
    tr->fixedWidth = buf.ReadByte();

    int bmpWidth = tr->maxCharWidth * 16;
    int bmpHeight = tr->charHeight * 14;
    char *tmpBitmap = new char [bmpWidth * bmpHeight];
    {
        int runVal = 1;
        int tmpBitmapOffset = 0;
        while (buf.currentPos < buf.len) {
            int runLeft = buf.ReadNibble();

            if (runLeft == 0) { // 0 is the escape token.
                runLeft = buf.ReadNibble();
                runLeft += buf.ReadNibble() << 4;
            }

            runVal = 1 - runVal;
            if (tmpBitmapOffset >= bmpWidth * bmpHeight) break;
            memset(tmpBitmap + tmpBitmapOffset, runVal, runLeft);
            tmpBitmapOffset += runLeft;
        }

        int pixelsLeft = bmpWidth * bmpHeight - tmpBitmapOffset;
        memset(tmpBitmap + tmpBitmapOffset, 0, pixelsLeft);
    }

    // Undo "up" prediction
    for (int y = 1; y < bmpHeight; y++) {
        for (int x = 0; x < bmpWidth; x++) {
            char up = tmpBitmap[(y - 1) * bmpWidth + x];
            tmpBitmap[y * bmpWidth + x] ^= up;
        }
    }

    // Allocate enough EncodedRuns to store the worst case for a glyph of this size.
    // Worst case is if the whole glyph is encoded as runs of one pixel. There has
    // to be a gap of one pixel between each run, so there can only be half as many
    // runs as there are pixels.
    unsigned tempRunsSize = tr->charHeight * (tr->maxCharWidth / 2 + 1);
    EncodedRun *tempRuns = new EncodedRun [tempRunsSize];

    // Run-length encode each ASCII character
    for (int i = 32; i < 256; i++)
    {
        // Get the size of this glyph
        int charWidth = tr->maxCharWidth;

        // Read back the bitmap and construct a run-length encoding
        memset(tempRuns, 0, tempRunsSize);
        EncodedRun *run = tempRuns;
        for (int y = 0; y < tr->charHeight; y++)
        {
            int x = 0;
            int bmpY = (i - 32) / 16 * tr->charHeight + y;
            while (x < tr->maxCharWidth)
            {
                // Skip blank pixels               
                int bmpX = i % 16 * tr->maxCharWidth + x;
                while (GetPixel(tmpBitmap, bmpWidth, bmpX, bmpY) == 0)
                {
                    x++;
                    if (x >= tr->maxCharWidth)
                        break;
                    bmpX++;
                }

                // Have we got to the end of the line?
                if (x >= tr->maxCharWidth)
                    continue;

                run->startX = x;
                run->startY = y;
                run->runLen = 0;

                // Count non-blank pixels
                while (GetPixel(tmpBitmap, bmpWidth, bmpX, bmpY) != 0)
                {
                    x++;
                    run->runLen++;
                    if (x >= tr->maxCharWidth)
                        break;
                    bmpX++;
                }

                run++;
            }
        }

        // Create the glyph to store the encoded runs we've made
        Glyph *glyph = new Glyph(charWidth);
        tr->glyphs[i] = glyph;

        // Copy the runs into the glyph
        glyph->m_numRuns = run - tempRuns;
        glyph->m_pixelRuns = new EncodedRun [glyph->m_numRuns];
        memcpy(glyph->m_pixelRuns, tempRuns, glyph->m_numRuns * sizeof(EncodedRun));
    }

    delete [] tempRuns;
    delete [] tmpBitmap;

    return tr;
}


DfFont *LoadFontFromFile(char const *filename, int pixHeight)
{
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    char buf[5];
    fread(buf, 1, 5, f);
    if (memcmp(buf, "dfbf\0", 5) != 0) return NULL;

    unsigned char numFonts = 0;
    fread(&numFonts, 1, 1, f);
    
    unsigned int filePffsets[257];
    fread(filePffsets, 4, numFonts, f);
    fseek(f, 0, SEEK_END);
    filePffsets[numFonts] = ftell(f);

    for (unsigned i = 0; i < numFonts; i++) {
        fseek(f, filePffsets[i], SEEK_SET);
        int maxWidth = fgetc(f);
        int pixHeight = fgetc(f);
        if (pixHeight == pixHeight) {
            fseek(f, filePffsets[i], SEEK_SET);
            int fontBinSize = filePffsets[i + 1] - filePffsets[i];
            char *fontBuf = new char[fontBinSize];
            fread(fontBuf, 1, fontBinSize, f);
            DfFont *fnt = LoadFontFromMemory((unsigned int *)fontBuf, fontBinSize / 4);
            delete[] fontBuf;
            return fnt;
        }
    }

    return NULL;
}


static int DrawTextSimpleClipped(DfFont *tr, DfColour col, DfBitmap *bmp, int _x, int y, char const *text, int maxChars)
{
    int x = _x;
    DfColour *startRow = bmp->pixels + y * bmp->width;
    int width = bmp->width;

    if (y + tr->charHeight < bmp->clipTop || y > bmp->clipBottom)
        return 0;

    for (int j = 0; text[j] && j < maxChars; j++)
    {
        if (x > bmp->clipRight)
            break;

        unsigned char c = text[j];
        Glyph *glyph = tr->glyphs[c];

        EncodedRun *rleBuf = glyph->m_pixelRuns;
        for (int i = 0; i < glyph->m_numRuns; i++)
        {
            int y3 = y + rleBuf->startY;
            if (y3 >= bmp->clipTop && y3 < bmp->clipBottom)
            {
                DfColour *thisRow = startRow + rleBuf->startY * width;
                for (unsigned i = 0; i < rleBuf->runLen; i++)
                {
                    int x3 = x + rleBuf->startX + i;
                    if (x3 >= bmp->clipLeft && x3 < bmp->clipRight)
                        thisRow[x3] = col;
                }
            }
            rleBuf++;
        }

        x += glyph->m_width;
    }

    return x - _x;
}


int DrawTextSimpleLen(DfFont *tr, DfColour col, DfBitmap *bmp, int _x, int y, char const *text, int maxChars)
{
    int x = _x;
    int width = bmp->width;

    if (x < bmp->clipLeft || y < bmp->clipTop || (y + tr->charHeight) > bmp->clipBottom)
        return DrawTextSimpleClipped(tr, col, bmp, _x, y, text, maxChars);

    DfColour *startRow = bmp->pixels + y * bmp->width;
    for (int j = 0; text[j] && j < maxChars; j++)
    {
        unsigned char c = text[j];
        if (x + (int)tr->glyphs[c]->m_width > bmp->clipRight)
            break;

        // Copy the glyph onto the stack for better cache performance. This increased the 
        // performance from 13.8 to 14.4 million chars per second. Madness.
        Glyph glyph = *tr->glyphs[c];
        EncodedRun *rleBuf = glyph.m_pixelRuns;
        for (int i = 0; i < glyph.m_numRuns; i++)
        {
            DfColour *startPixel = startRow + rleBuf->startY * width + rleBuf->startX + x;
            for (unsigned i = 0; i < rleBuf->runLen; i++)
                startPixel[i] = col;
            rleBuf++;
        }

        x += glyph.m_width;
    }

    return x - _x;
}


int DrawTextSimple(DfFont *f, DfColour col, DfBitmap *bmp, int x, int y, char const *text)
{
    return DrawTextSimpleLen(f, col, bmp, x, y, text, 9999);
}


int DrawTextLeft(DfFont *tr, DfColour c, DfBitmap *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimple(tr, c, bmp, x, y, buf);
}


int DrawTextRight(DfFont *tr, DfColour c, DfBitmap *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, x - width, y, buf);
}


int DrawTextCentre(DfFont *tr, DfColour c, DfBitmap *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(tr, buf);
    return DrawTextSimple(tr, c, bmp, x - width/2, y, buf);
}


int GetTextWidth(DfFont *tr, char const *text, int len)
{
    len = IntMin((int)strlen(text), len);
    if (tr->fixedWidth)
    {
        return len * tr->maxCharWidth;
    }
    else
    {
        int width = 0;
        for (int i = 0; i < len; i++)
            width += tr->glyphs[text[i] & 0xff]->m_width;

        return width;
    }
}

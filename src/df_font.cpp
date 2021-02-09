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


static int GetPixel(char *tmpBitmap, int bmpWidth, int x, int y) 
{
    return tmpBitmap[y * bmpWidth + x];
}


struct MemBuf 
{
    unsigned char *data;
    int len;
    int currentPos;
    int currentByte;
    int hiNibbleNext;

    MemBuf(unsigned *_data, int _len) 
    {
        data = (unsigned char *)_data;
        len = _len;
        currentPos = 0;
        currentByte = 0;
        hiNibbleNext = 0;
    }

    int ReadByte() 
    {
        if (currentPos >= len) return 0;
        currentByte = data[currentPos];
        currentPos++;
        return currentByte;
    }

    int ReadNibble() 
    {
        if (hiNibbleNext) {
            hiNibbleNext = 0;
            return currentByte >> 4;
        }

        ReadByte();
        hiNibbleNext = 1;
        return currentByte & 0xf;
    }
};


// ****************************************************************************
// Public Functions
// ****************************************************************************

DfFont *LoadFontFromMemory(void *_buf, int bufLen)
{
    MemBuf buf((unsigned int *)_buf, bufLen);
    DfFont *fnt = new DfFont;
    memset(fnt, 0, sizeof(DfFont));

    fnt->maxCharWidth = buf.ReadByte();
    fnt->charHeight = buf.ReadByte();
    fnt->fixedWidth = !buf.ReadByte();

    char glyphWidths[224];
    if (!fnt->fixedWidth)
    {
        for (int i = 0; i < 224; i++) glyphWidths[i] = buf.ReadByte();
    }
    else 
    {
        for (int i = 0; i < 224; i++) glyphWidths[i] = fnt->maxCharWidth;
    }

    // Decode the RLE data into a bitmap with one char per pixel.
    int bmpWidth = fnt->maxCharWidth * 16;
    int bmpHeight = fnt->charHeight * 14;
    char *tmpBitmap = new char [bmpWidth * bmpHeight];
    {
        int runVal = 1;
        int tmpBitmapOffset = 0;
        while (buf.currentPos < buf.len) 
        {
            int runLeft = buf.ReadNibble();

            if (runLeft == 0) // 0 is the escape token.
            {
                runLeft = buf.ReadNibble();
                runLeft += buf.ReadNibble() << 4;
            }

            runVal = 1 - runVal;
            DebugAssert((tmpBitmapOffset + runLeft) < bmpWidth * bmpHeight);
            memset(tmpBitmap + tmpBitmapOffset, runVal, runLeft);
            tmpBitmapOffset += runLeft;
        }

        int pixelsLeft = bmpWidth * bmpHeight - tmpBitmapOffset;
        DebugAssert(pixelsLeft >= 0);
        memset(tmpBitmap + tmpBitmapOffset, 0, pixelsLeft);
    }

    // Undo "up" prediction
    for (int y = 1; y < bmpHeight; y++) 
    {
        for (int x = 0; x < bmpWidth; x++) 
        {
            char up = tmpBitmap[(y - 1) * bmpWidth + x];
            tmpBitmap[y * bmpWidth + x] ^= up;
        }
    }

    // Allocate enough EncodedRuns to store the worst case for a glyph of this size.
    // Worst case is if the whole glyph is encoded as runs of one pixel. There has
    // to be a gap of one pixel between each run, so there can only be half as many
    // runs as there are pixels.
    unsigned tempRunsSize = fnt->charHeight * (fnt->maxCharWidth / 2 + 1);
    EncodedRun *tempRuns = new EncodedRun [tempRunsSize];

    // Run-length encode each ASCII character
    for (int i = 32; i < 256; i++)
    {
        // Read back the bitmap and construct a run-length encoding
        memset(tempRuns, 0, tempRunsSize);
        EncodedRun *run = tempRuns;
        for (int y = 0; y < fnt->charHeight; y++)
        {
            int x = 0;
            int bmpY = (i - 32) / 16 * fnt->charHeight + y;
            while (x < fnt->maxCharWidth)
            {
                // Skip blank pixels               
                int bmpX = i % 16 * fnt->maxCharWidth + x;
                while (GetPixel(tmpBitmap, bmpWidth, bmpX, bmpY) == 0)
                {
                    x++;
                    if (x >= fnt->maxCharWidth)
                        break;
                    bmpX++;
                }

                // Have we got to the end of the line?
                if (x >= fnt->maxCharWidth)
                    continue;

                run->startX = x;
                run->startY = y;
                run->runLen = 0;

                // Count non-blank pixels
                while (GetPixel(tmpBitmap, bmpWidth, bmpX, bmpY) != 0)
                {
                    x++;
                    run->runLen++;
                    if (x >= fnt->maxCharWidth)
                        break;
                    bmpX++;
                }

                run++;
            }
        }

        // Create the glyph to store the encoded runs we've made
        Glyph *glyph = new Glyph(glyphWidths[i - 32]);
        fnt->glyphs[i] = glyph;

        // Copy the runs into the glyph
        glyph->m_numRuns = run - tempRuns;
        glyph->m_pixelRuns = new EncodedRun [glyph->m_numRuns];
        memcpy(glyph->m_pixelRuns, tempRuns, glyph->m_numRuns * sizeof(EncodedRun));
    }

    // Copy the space glyph into all the unprintable chars, so that DrawTextSimpleLen()
    // doesn't crash when asked to draw one.
    for (int i = 0; i < 32; i++)
        fnt->glyphs[i] = fnt->glyphs[' '];

    delete [] tempRuns;
    delete [] tmpBitmap;

    return fnt;
}


int ListFontSizesInFile(char const *filename, int result[16])
{
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;

    unsigned char numFonts = 0;
    char buf[5];
    fread(buf, 1, 5, f);
    if (memcmp(buf, "dfbf\0", 5) != 0) goto error;

    fread(&numFonts, 1, 1, f);
    if (numFonts > 15) numFonts = 15;

    unsigned int fileOffsets[257];
    if (fread(fileOffsets, 4, numFonts, f) != numFonts) goto error;
    if (fseek(f, 0, SEEK_END) != 0) goto error;
    fileOffsets[numFonts] = ftell(f);
    if (fileOffsets[numFonts] == -1) goto error;

    for (unsigned i = 0; i < numFonts; i++) {
        int err = fseek(f, fileOffsets[i] + 1, SEEK_SET); // +1 skips the max width field.
        if (err != 0) goto error;
        result[i] = fgetc(f);
    }

    fclose(f);
    return numFonts;

error:
    fclose(f);
    return -1;
}


DfFont *LoadFontFromFile(char const *filename, int pixHeight)
{
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    unsigned char numFonts = 0;
    char buf[5];
    fread(buf, 1, 5, f);
    if (memcmp(buf, "dfbf\0", 5) != 0) goto error;

    fread(&numFonts, 1, 1, f);
    
    unsigned int fileOffsets[257];
    if (fread(fileOffsets, 4, numFonts, f) != numFonts) goto error;
    if (fseek(f, 0, SEEK_END) != 0) goto error;
    fileOffsets[numFonts] = ftell(f);
    if (fileOffsets[numFonts] == -1) goto error;

    for (unsigned i = 0; i < numFonts; i++) {
        int err = fseek(f, fileOffsets[i] + 1, SEEK_SET); // +1 skips the max width field.
        if (err != 0) goto error;

        int thisPixHeight = fgetc(f);
        if (thisPixHeight == pixHeight) {
            err = fseek(f, fileOffsets[i], SEEK_SET);
            if (err != 0) goto error;

            int fontBinSize = fileOffsets[i + 1] - fileOffsets[i];
            char *fontBuf = new char[fontBinSize];
            if (fread(fontBuf, 1, fontBinSize, f) != fontBinSize) goto error;
            DfFont *fnt = LoadFontFromMemory((unsigned int *)fontBuf, fontBinSize);
            delete[] fontBuf;
            fclose(f);
            return fnt;
        }
    }

error:
    fclose(f);
    return NULL;
}


static int DrawTextSimpleClipped(DfFont *fnt, DfColour col, DfBitmap *bmp, int _x, int y, char const *text, int maxChars)
{
    int x = _x;
    DfColour *startRow = bmp->pixels + y * bmp->width;
    int width = bmp->width;

    if (y + fnt->charHeight < bmp->clipTop || y > bmp->clipBottom)
        return 0;

    for (int j = 0; j < maxChars && text[j]; j++)
    {
        if (x > bmp->clipRight)
            break;

        unsigned char c = text[j];
        Glyph *glyph = fnt->glyphs[c];

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

int DrawTextSimpleLen(DfFont *fnt, DfColour col, DfBitmap *bmp, int _x, int y, char const *text, int maxChars)
{
    int x = _x;
    int width = bmp->width;

    if (x < bmp->clipLeft || y < bmp->clipTop || (y + fnt->charHeight) > bmp->clipBottom)
        return DrawTextSimpleClipped(fnt, col, bmp, _x, y, text, maxChars);

    DfColour *startRow = bmp->pixels + y * bmp->width;
    for (int j = 0; j < maxChars && text[j]; j++)
    {
        unsigned char c = text[j];
        if (x + (int)fnt->glyphs[c]->m_width > bmp->clipRight)
        {
            x += DrawTextSimpleClipped(fnt, col, bmp, x, y, text + j, 1);
            break;
        }

        // Copy the glyph onto the stack for better cache performance. This increased the 
        // performance from 13.8 to 14.4 million chars per second. Madness.
        Glyph glyph = *fnt->glyphs[c];
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


int DrawTextLeft(DfFont *fnt, DfColour c, DfBitmap *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, text);
    vsprintf(buf, text, ap);
    return DrawTextSimple(fnt, c, bmp, x, y, buf);
}


int DrawTextRight(DfFont *fnt, DfColour c, DfBitmap *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(fnt, buf);
    return DrawTextSimple(fnt, c, bmp, x - width, y, buf);
}


int DrawTextCentre(DfFont *fnt, DfColour c, DfBitmap *bmp, int x, int y, char const *text, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, text);
    vsprintf(buf, text, ap);

    int width = GetTextWidth(fnt, buf);
    return DrawTextSimple(fnt, c, bmp, x - width/2, y, buf);
}


int GetTextWidth(DfFont *fnt, char const *text, int len)
{
    len = IntMin((int)strlen(text), len);
    if (fnt->fixedWidth)
    {
        return len * fnt->maxCharWidth;
    }
    else
    {
        int width = 0;
        for (int i = 0; i < len; i++)
            width += fnt->glyphs[text[i] & 0xff]->m_width;

        return width;
    }
}

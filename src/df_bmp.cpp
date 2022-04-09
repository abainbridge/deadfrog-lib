#include "df_bmp.h"

#include "df_bitmap.h"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>


#define BMP_RGB             0
#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40


struct BitmapFileHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offBits;
};


struct BitmapInfoHeader {
    uint32_t width;
    uint32_t height;
    uint16_t bitCount;
    uint32_t compression;
};


struct WinBmpInfoHeader {
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitCount;
    uint32_t compression;
    uint32_t sizeImage;
    uint32_t xPelsPerMeter;
    uint32_t biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};


// ****************************************************************************
// Private Functions
// ****************************************************************************

static uint8_t ReadU8(FILE *file) {
    return fgetc(file);
}


static short ReadS16(FILE *file) {
    int b1 = fgetc(file);
    int b2 = fgetc(file);
    return ((b2 << 8) | b1);
}


static int ReadS32(FILE *file) {
    int b1 = fgetc(file);
    int b2 = fgetc(file);
    int b3 = fgetc(file);
    int b4 = fgetc(file);
    return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}


// Reads a BMP file header and check that it has the BMP magic number.
static void ReadBmpFileHeader(FILE *f, BitmapFileHeader *fileheader) {
    fileheader->type = ReadS16(f);
    fileheader->size = ReadS32(f);
    fileheader->reserved1 = ReadS16(f);
    fileheader->reserved2 = ReadS16(f);
    fileheader->offBits = ReadS32(f);

    ReleaseAssert(fileheader->type == 19778, "BMP file seems corrupt");
}


// Reads information from a BMP file header.
static void ReadWinBmpInfoHeader(FILE *f, BitmapInfoHeader *infoheader) {
    WinBmpInfoHeader win_infoheader;

    win_infoheader.width = ReadS32(f);
    win_infoheader.height = ReadS32(f);
    win_infoheader.planes = ReadS16(f);
    win_infoheader.bitCount = ReadS16(f);
    win_infoheader.compression = ReadS32(f);
    win_infoheader.sizeImage = ReadS32(f);
    win_infoheader.xPelsPerMeter = ReadS32(f);
    win_infoheader.biYPelsPerMeter = ReadS32(f);
    win_infoheader.biClrUsed = ReadS32(f);
    win_infoheader.biClrImportant = ReadS32(f);

    infoheader->width = win_infoheader.width;
    infoheader->height = win_infoheader.height;
    infoheader->bitCount = win_infoheader.bitCount;
    infoheader->compression = win_infoheader.compression;
}


static void ReadBmpPalette(int ncols, DfColour pal[256], FILE *f) {
    for (int i = 0; i < ncols; i++) {
        pal[i].b = ReadU8(f);
        pal[i].g = ReadU8(f);
        pal[i].r = ReadU8(f);
        pal[i].a = 255;
        ReadU8(f);
    }
}


static void SkipPadding(FILE *f, int width, int pixels_per_byte) {
    // Skip any padding until we've read a multiple of 4 bytes.
    int num_bytes = (width + pixels_per_byte - 1) / pixels_per_byte;
    while (num_bytes & 3) {
        ReadU8(f);
        num_bytes++;
    }
}


static void Read1BitLine(DfBitmap *bitmap, int length, FILE *f, DfColour pal[256], int y) {
    for (int x = 0; x < length; x += 8) {
        uint8_t i = ReadU8(f);
        for (int j = 0; j < 8; j++) {
            int idx = (i >> 7) & 1;
            PutPix(bitmap, x + j, y, pal[idx]);
            i <<= 1;
        }
    }

    SkipPadding(f, length, 8);
}


static void Read4BitLine(DfBitmap *bitmap, int length, FILE *f, DfColour pal[256], int y) {
    for (int x = 0; x < length; x += 2) {
        uint8_t i = ReadU8(f);
        unsigned idx1 = i & 15;
        unsigned idx2 = (i >> 4) & 15;
        PutPix(bitmap, x+1, y, pal[idx1]);
        PutPix(bitmap, x, y, pal[idx2]);
    }

    SkipPadding(f, length, 2);
}


static void Read8BitLine(DfBitmap *bitmap, int length, FILE *f, DfColour pal[256], int y) {
    for (int x = 0; x < length; ++x) {
        uint8_t i = ReadU8(f);
        PutPix(bitmap, x, y, pal[i]);
    }

    SkipPadding(f, length, 1);
}


static void Read24BitLine(DfBitmap *bitmap, int length, FILE *f, int y) {
    DfColour c;
    int nbytes = 0;
    c.a = 255;

    for (int i = 0; i < length; i++) {
        c.b = ReadU8(f);
        c.g = ReadU8(f);
        c.r = ReadU8(f);
        PutPix(bitmap, i, y, c);
        nbytes += 3;
    }

    for (int padding = (4 - nbytes) & 3; padding; --padding) 
        ReadU8(f);
}


// ****************************************************************************
// Public Functions
// ****************************************************************************

DfBitmap *LoadBmp(char const *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    BitmapFileHeader fileheader;
    ReadBmpFileHeader(f, &fileheader);

    uint32_t infoHeaderSize = ReadS32(f);
    ReleaseAssert(infoHeaderSize == WININFOHEADERSIZE, "Bitmap header size invalid (%i bytes). Is it a Windows BMP?", infoHeaderSize); 

    BitmapInfoHeader infoheader;
    ReadWinBmpInfoHeader(f, &infoheader);
    ReleaseAssert(infoheader.compression == BMP_RGB, "Bitmap loader does not support RLE compressed bitmaps"); 

    int ncol = (fileheader.offBits - 54) / 4; // Calc num colours in palette.
    DfColour palette[256];
    ReadBmpPalette(ncol, palette, f);

    DfBitmap *bitmap = BitmapCreate(infoheader.width, infoheader.height);

    // Read the image
    for (int i = 0; i < (int)infoheader.height; ++i) {
        int y = bitmap->height - i - 1;
        switch (infoheader.bitCount) {
        case 1:
            Read1BitLine(bitmap, infoheader.width, f, palette, y);
            break;
        case 4:
            Read4BitLine(bitmap, infoheader.width, f, palette, y);
            break;
        case 8:
            Read8BitLine(bitmap, infoheader.width, f, palette, y);
            break;
        case 24:
            Read24BitLine(bitmap, infoheader.width, f, y);
            break;
        default:
            DebugAssert(0);
            break;
        }
    }

    fclose(f);
    return bitmap;
}


bool SaveBmp(DfBitmap *bmp, char const *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f)
        return false;

    int w = bmp->width;
    int h = bmp->height;
    int fileSize = 54 + 3*w*h;  //w is your image width, h is image height, both int
    uint8_t *img = new uint8_t [3*w*h];

    for (int x = 0; x < w; x++) {
        for (int j = 0; j < h; j++) {
            int y = (h-1) - j;
            DfColour c = GetPix(bmp, x, y);
            img[(x+y*w)*3+2] = c.r;
            img[(x+y*w)*3+1] = c.g;
            img[(x+y*w)*3+0] = c.b;
        }
    }

    uint8_t fileHeader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    uint8_t infoHeader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    uint8_t bmppad[3] = {0,0,0};

    fileHeader[2] = (uint8_t)(fileSize    );
    fileHeader[3] = (uint8_t)(fileSize>> 8);
    fileHeader[4] = (uint8_t)(fileSize>>16);
    fileHeader[5] = (uint8_t)(fileSize>>24);

    infoHeader[4] = (uint8_t)w;
    infoHeader[5] = (uint8_t)(w >> 8);
    infoHeader[6] = (uint8_t)(w >> 16);
    infoHeader[7] = (uint8_t)(w >> 24);
    infoHeader[8] = (uint8_t)h;
    infoHeader[9] = (uint8_t)(h >> 8);
    infoHeader[10] = (uint8_t)(h >> 16);
    infoHeader[11] = (uint8_t)(h >> 24);

    fwrite(fileHeader, 1, 14, f);
    fwrite(infoHeader, 1, 40, f);
    for (int i = 0; i < h; i++) {
        fwrite(img+(w*(h-i-1)*3), 3, w, f);
        fwrite(bmppad, 1, (4-(w*3)%4)%4, f);
    }
    fclose(f);

    delete[] img;
    return true;
}

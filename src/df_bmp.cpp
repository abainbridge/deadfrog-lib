#include "df_bitmap.h"

#include <memory.h>
#include <stdio.h>


#define BMP_RGB				0
#define OS2INFOHEADERSIZE  12
#define WININFOHEADERSIZE  40


struct BitmapFileHeader
{
	unsigned short bfType;
	unsigned long  bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long  bfOffBits;
};


struct BitmapInfoHeader
{
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biBitCount;
	unsigned long  biCompression;
};


struct WinBmpInfoHeader
{
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long  biCompression;
	unsigned long  biSizeImage;
	unsigned long  biXPelsPerMeter;
	unsigned long  biYPelsPerMeter;
	unsigned long  biClrUsed;
	unsigned long  biClrImportant;
};


// ****************************************************************************
// Private Functions
// ****************************************************************************

static unsigned char ReadU8(FILE *file)
{
    return fgetc(file);
}


static short ReadS16(FILE *file)
{
    int b1 = fgetc(file);
    int b2 = fgetc(file);
    return ((b2 << 8) | b1);
}


static int ReadS32(FILE *file)
{
    int b1 = fgetc(file);
    int b2 = fgetc(file);
    int b3 = fgetc(file);
    int b4 = fgetc(file);
    return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}


// Reads a BMP file header and check that it has the BMP magic number.
static void ReadBmpFileHeader(FILE *f, BitmapFileHeader *fileheader)
{
	fileheader->bfType = ReadS16(f);
	fileheader->bfSize = ReadS32(f);
	fileheader->bfReserved1 = ReadS16(f);
	fileheader->bfReserved2 = ReadS16(f);
	fileheader->bfOffBits = ReadS32(f);

	ReleaseAssert(fileheader->bfType == 19778, "BMP file seems corrupt");
}


// Reads information from a BMP file header.
void ReadWinBmpInfoHeader(FILE *f, BitmapInfoHeader *infoheader)
{
	WinBmpInfoHeader win_infoheader;

	win_infoheader.biWidth = ReadS32(f);
	win_infoheader.biHeight = ReadS32(f);
	win_infoheader.biPlanes = ReadS16(f);
	win_infoheader.biBitCount = ReadS16(f);
	win_infoheader.biCompression = ReadS32(f);
	win_infoheader.biSizeImage = ReadS32(f);
	win_infoheader.biXPelsPerMeter = ReadS32(f);
	win_infoheader.biYPelsPerMeter = ReadS32(f);
	win_infoheader.biClrUsed = ReadS32(f);
	win_infoheader.biClrImportant = ReadS32(f);

	infoheader->biWidth = win_infoheader.biWidth;
	infoheader->biHeight = win_infoheader.biHeight;
	infoheader->biBitCount = win_infoheader.biBitCount;
	infoheader->biCompression = win_infoheader.biCompression;
}


void ReadBmpPalette(int ncols, DfColour pal[256], FILE *f)
{
	for (int i = 0; i < ncols; i++) 
	{
	    pal[i].b = ReadU8(f);
	    pal[i].g = ReadU8(f);
	    pal[i].r = ReadU8(f);
		pal[i].a = 255;
        ReadU8(f);
	}
}


// Support function for reading the 4 bit bitmap file format.
void Read4BitLine(DfBitmap *bitmap, int length, FILE *f, DfColour pal[256], int y)
{
	for (int x = 0; x < length; x += 2) 
	{
		unsigned char i = ReadU8(f);
		unsigned idx1 = i & 15;
		unsigned idx2 = (i >> 4) & 15;
		PutPix(bitmap, x+1, y, pal[idx1]);
		PutPix(bitmap, x, y, pal[idx2]);
	}
}


// Support function for reading the 8 bit bitmap file format.
void Read8BitLine(DfBitmap *bitmap, int length, FILE *f, DfColour pal[256], int y)
{
	for (int x = 0; x < length; ++x) 
	{
		unsigned char i = ReadU8(f);
		PutPix(bitmap, x, y, pal[i]);
	}
    ReadS16(f);
    ReadU8(f);
}


// Support function for reading the 24 bit bitmap file format
void Read24BitLine(DfBitmap *bitmap, int length, FILE *f, int y)
{
	DfColour c;
	int nbytes = 0;
	c.a = 255;

	for (int i=0; i<length; i++) 
	{
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

DfBitmap *LoadBmp(char const *filename)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
        return NULL;

    BitmapFileHeader fileheader;
    ReadBmpFileHeader(f, &fileheader);

    unsigned long biSize = ReadS32(f);

    BitmapInfoHeader infoheader;
    DfColour palette[256];
    if (biSize == WININFOHEADERSIZE) 
    {
        ReadWinBmpInfoHeader(f, &infoheader);
        int ncol = (fileheader.bfOffBits - 54) / 4; // compute number of colors recorded
        ReadBmpPalette(ncol, palette, f);
    }
    ReleaseAssert(biSize != OS2INFOHEADERSIZE, "Bitmap loader does not support OS/2 format bitmaps");
    ReleaseAssert(infoheader.biCompression == BMP_RGB, "Bitmap loader does not support RLE compressed bitmaps"); 

    DfBitmap *bitmap = CreateBitmapRGBA(infoheader.biWidth, infoheader.biHeight);

    // Read the image
    for (int i = 0; i < (int)infoheader.biHeight; ++i) 
    {
        int y = bitmap->height - i - 1;
        switch (infoheader.biBitCount)
        {
        case 4:
            Read4BitLine(bitmap, infoheader.biWidth, f, palette, y);
            break;
        case 8:
            Read8BitLine(bitmap, infoheader.biWidth, f, palette, y);
            break;
        case 24:
            Read24BitLine(bitmap, infoheader.biWidth, f, y);
            break;
        default:
            DebugAssert(0);
            break;
        }
    }

    fclose(f);
    return bitmap;
}


bool SaveBmp(DfBitmap *bmp, char const *filename)
{
    int w = bmp->width;
    int h = bmp->height;
    int filesize = 54 + 3*w*h;  //w is your image width, h is image height, both int
    unsigned char *img = new unsigned char [3*w*h];
    memset(img, 0, sizeof(img));

    for(int x = 0; x < w; x++)
    {
        for(int j = 0; j < h; j++)
        {
            int y = (h-1) - j;
            DfColour c = GetPix(bmp, x, y);
            img[(x+y*w)*3+2] = c.r;
            img[(x+y*w)*3+1] = c.g;
            img[(x+y*w)*3+0] = c.b;
        }
    }

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       w    );
    bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       w>>16);
    bmpinfoheader[ 7] = (unsigned char)(       w>>24);
    bmpinfoheader[ 8] = (unsigned char)(       h    );
    bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
    bmpinfoheader[10] = (unsigned char)(       h>>16);
    bmpinfoheader[11] = (unsigned char)(       h>>24);

    FILE *f = fopen(filename, "wb");
    fwrite(bmpfileheader, 1, 14, f);
    fwrite(bmpinfoheader, 1, 40, f);
    for(int i=0; i<h; i++)
    {
        fwrite(img+(w*(h-i-1)*3), 3, w, f);
        fwrite(bmppad, 1, (4-(w*3)%4)%4, f);
    }
    fclose(f);

    delete [] img;
    return true;
}

// g++ bmp_to_fnt.c -o bmp_to_fnt -g -I../src ../src/df_bitmap.cpp ../src/df_bmp.cpp ../src/df_colour.cpp ../src/df_common_linux.cpp

#include "df_bitmap.h"
#include "df_bmp.h"
#include "df_common.h"
#include <stdio.h>
#include <stdlib.h>


static void write(int runLen) {
    static int column = 0;
    column += printf("%i,", runLen);
    if (column > 95) {
        printf("\n ");
        column = 0;
    }
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        puts("Usage: bmp_to_fnt <your.bmp> <glyph height> <glyph width>");
        puts("Outputs the font definition as a C code that can be included in your project.");
        puts("Params:");
        puts("  your.bmp - Your bitmap should have 16 columns and 14 rows of glyphs.");
        puts("             The first (top left) glyph should be ascii 32. The last");
        puts("             should be ascii 255.");
        puts("             The bitmap size should be 16 * glyph width, 14 * glyph height");
        puts("  glyph height - In pixels");
        puts("  glyph width - In pixels.");
        return 0;
    }

    const char *bmp_name = argv[1];
    int glyph_width = strtoul(argv[2], NULL, 10);
    int glyph_height = strtoul(argv[3], NULL, 10);

    DfBitmap *bmp = LoadBmp(bmp_name);
    ReleaseAssert(bmp, "Couldn't load bitmap '%s'.", bmp_name);

    ReleaseAssert(glyph_width < 256, "Glyph width %i too large. Maximum is 255.", glyph_width);
    ReleaseAssert(glyph_height < 256, "Glyph height %i too large. Maximum is 255.", glyph_height);
    ReleaseAssert(glyph_width * 16 == bmp->width, "Bitmap has the wrong width. It is %i and should be %i\n",
        bmp->width, 16 * glyph_width);
    ReleaseAssert(glyph_height * 14 == bmp->height, "Bitmap has the wrong height. It is %i and should be %i\n",
        bmp->height, 14 * glyph_height);

    printf("unsigned char %s[] = {\n ", argv[1]);
    write((int)'d'); // \ 
    write((int)'f'); //  } Format magic
    write((int)'b'); //  } marker.
    write((int)'f'); // /
    write(0);   // Format version.
    write(glyph_width);
    write(glyph_height);

    DfColour runVal = g_colourBlack;
    int runLen = 0;
    for (int y = 0; y < bmp->height; y++) {
        for (int x = 0; x < bmp->width; x++) {
            if (GetPix(bmp, x, y).c == runVal.c) {
                runLen++;
                if (runLen >= 255) {
                    write(runLen);
                    runLen = 0;
                }
            }
            else {
                write(runLen);
                runLen = 1;
                runVal = GetPix(bmp, x, y);
            }
        }
    }

    printf("\n};\n");
}

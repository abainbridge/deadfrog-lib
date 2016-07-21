#include "df_time.h"
#include "df_input.h"
#include "df_polygon.h"
#include "df_font.h"
#include "df_font_aa.h"
#include "df_window.h"

#include <string.h>


#include "df_master_glyph.h"

inline unsigned GetRand()
{
    static unsigned r = 0x12345671;
    return r = ((r * 214013 + 2531011) >> 16) & 0xffff;
}


double CalcMillionPixelsPerSec(DfBitmap *bmp)
{
    unsigned iterations = 1000 * 1000 * 10;
    double startTime = DfGetTime();
    for (unsigned i = 0; i < iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 128;
        PutPixUnclipped(bmp, r & 255, (r >> 8) & 255, c);
    }
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations;
    return (numPixels / duration) / 1e6;
}


double CalcMillionHLinePixelsPerSec(DfBitmap *bmp)
{
    unsigned iterations = 1000 * 1000 * 10;
    double startTime = DfGetTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 255;
        HLine(bmp, r & 255, (r >> 8) & 255, lineLen, c);
    }
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations * lineLen;
    return (numPixels / duration) / 1e6;
}


double CalcMillionVLinePixelsPerSec(DfBitmap *bmp)
{
    unsigned iterations = 1000 * 1000 * 10;
    double startTime = DfGetTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 255;
        VLine(bmp, r & 255, (r >> 8) & 255, lineLen, c);
    }
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations * lineLen;
    return (numPixels / duration) / 1e6;
}


double CalcBillionRectFillPixelsPerSec(DfBitmap *bmp)
{
    unsigned iterations = 1000 * 100;
    double startTime = DfGetTime();
    for (unsigned i = 0; i < iterations; i++)
        RectFill(bmp, 0, 0, 300, 300, g_colourWhite);
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numPixels = 300.0 * 300.0 * (double)iterations;
    return (numPixels / duration) / 1e9;
}


double CalcMillionLinePixelsPerSec(DfBitmap *bmp)
{
    unsigned iterations = 1000 * 600;
    double startTime = DfGetTime();
    for (unsigned i = 0; i < iterations; ++i)
    {
        DrawLine(bmp, 0, 0, 20,  300, g_colourWhite);   // 320 long, nice slope
        DrawLine(bmp, 0, 0, 300, 20, g_colourWhite);
        DrawLine(bmp, 0, 0, 1,   15, g_colourWhite);    // 16 long, nice slope
        DrawLine(bmp, 0, 0, 15,  1, g_colourWhite);
        DrawLine(bmp, 0, 0, 150, 151, g_colourWhite);   // 151 long, annoying slope
        DrawLine(bmp, 0, 0, 151, 150, g_colourWhite);
    }
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numPixels = (320 + 16 + 151) * 2 * iterations;
    return (numPixels / duration) / 1e6;
}


double CalcMillionCharsPerSec(DfBitmap *bmp, DfFont *font)
{
    static char const *str = "Here's some interesting text []£# !";
    unsigned iterations = 1000 * 100;
    double startTime = DfGetTime();
    for (unsigned i = 0; i < iterations; i++)
        DrawTextSimple(font, g_colourWhite, bmp, 10, 10, str);
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numChars = iterations * (double)strlen(str);
    return (numChars / duration) / 1000000.0;
}


double CalcMillionAaCharsPerSec(DfBitmap *bmp, DfFontAa *font)
{
    static char const *str = "Here's some interesting text []£# !";
    unsigned iterations = 1000 * 1;
    double startTime = DfGetTime();
    for (unsigned i = 0; i < iterations; i++)
        DrawTextSimpleAa(font, Colour(255,255,255,60), bmp, 10, 10, 10, str);
    double endTime = DfGetTime();

//    RectFill(bmp, 10, 10, 500, font->charHeight, g_colourBlack);
    DrawTextSimpleAa(font, Colour(255,255,255,255), bmp, 10, 10, 10, str);

    double duration = endTime - startTime;
    double numChars = iterations * (double)strlen(str);
    return (numChars / duration) / 1000000.0;
}


#define DRAW_POLYGON(pointList, Color, x, y)              \
    Polygon.numPoints = sizeof(pointList) / sizeof(Point);   \
    Polygon.points = pointList;                         \
    FillConvexPolygon(bmp, &Polygon, g_colourWhite, x, y);


double CalcBillionPolyFillPixelsPerSec(DfBitmap *bmp)
{
    unsigned iterations = 100 * 20;
    double startTime = DfGetTime();
    for (unsigned k = 0; k < iterations; k++)
    {
        PointListHeader Polygon;
        static Point ScreenRectangle[] = {{340,10},{380,10},{380,200},{340,200}};
        static Point Hexagon[] = {{190,250},{100,210},{10,250},{10,350},{100,390},{190,350}};
        static Point Triangle1[] = {{30,0},{15,20},{0,0}};
        static Point Triangle2[] = {{30,20},{15,0},{0,20}};
        static Point Triangle3[] = {{0,20},{20,10},{0,0}};
        static Point Triangle4[] = {{20,20},{20,0},{0,10}};

        DRAW_POLYGON(ScreenRectangle, 3, 0, 1);

        /* Draw adjacent triangles across the top half of the screen */
        int i, j;
        for (j=0; j<=80; j+=20) {
            for (i=0; i<290; i += 30) {
                DRAW_POLYGON(Triangle1, 2, i, j);
                DRAW_POLYGON(Triangle2, 4, i+15, j);
            }
        }

        /* Draw adjacent triangles across the bottom half of the screen */
        for (j=100; j<=170; j+=20) {
            /* Do a row of pointing-right triangles */
            for (i=0; i<290; i += 20) {
                DRAW_POLYGON(Triangle3, 40, i, j);
            }
            /* Do a row of pointing-left triangles halfway between one row
            of pointing-right triangles and the next, to fit between */
            for (i=0; i<290; i += 20) {
                DRAW_POLYGON(Triangle4, 1, i, j+10);
            }
        }

        DRAW_POLYGON(Hexagon, i, 0, 0);
    }
    
    double endTime = DfGetTime();
    double duration = endTime - startTime;
    double numPixels = 87000.0 * (double)iterations;
    return (numPixels / duration) / 1e9;
}


#define END_TEST \
    textY += yInc; \
    QuickBlit(g_window->bmp, 400, textY, backBmp); \
    UpdateWin(); \
    InputManagerAdvance(); \
    if (g_window->windowClosed || g_input.keyDowns[KEY_ESC]) return; \
    ClearBitmap(backBmp, g_colourBlack);


void BenchmarkMain()
{
	CreateWin(1024, 768, WT_WINDOWED, "Benchmark");
    DfFont *font = DfCreateFont("Courier New", 10, 4);

    ClearBitmap(g_window->bmp, g_colourBlack);

    double startTime = DfGetTime();
    DfFontAa *fontAa = CreateFontAa("Lucida Console", 4);
//     DfFontAa *fontAa2 = CreateDfFontAa("Lucida Console", 4);
//     DfFontAa *fontAa3 = CreateDfFontAa("Lucida Console", 4);
//     DfFontAa *fontAa4 = CreateDfFontAa("Courier New", 4);
//     DfFontAa *fontAa5 = CreateDfFontAa("Times New Roman", 4);
    double aaFontCreationTime = DfGetTime() - startTime;

    DfBitmap *backBmp = DfCreateBitmap(800, 600);

    int textY = 10;
    int yInc = font->charHeight * 3;

    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
        "Font creation took %.2f sec", aaFontCreationTime);
    textY += 15;

//     DrawTextSimpleAa(fontAa, g_colourWhite, g_window->bmp, 10, textY, 10, "Here's some interesting text. See! What?");
//     textY += 20;
//     DrawTextSimpleAa(fontAa2, g_colourWhite, g_window->bmp, 10, textY, "Here's some interesting text. See! What?");
//     textY += 20;
//     DrawTextSimpleAa(fontAa3, g_colourWhite, g_window->bmp, 10, textY, "Here's some interesting text. See! What?");
//     textY += 20;
//     DrawTextSimpleAa(fontAa4, g_colourWhite, g_window->bmp, 10, textY, "Here's some interesting text. See! What?");
//     textY += 20;
//     DrawTextSimpleAa(fontAa5, g_colourWhite, g_window->bmp, 10, textY, "Here's some interesting text. See! What?");
//     textY += 20;

    // Put pixel
    double score = CalcMillionPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, 
        "Million putpixels per sec = %.1f", score);
    END_TEST;

    // HLine draw
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcMillionHLinePixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
        "Million Hline pixels per sec = %.2f", score);
    END_TEST;

//     // VLine draw
//     ClearBitmap(backBmp, g_colourBlack);
//     score = CalcMillionVLinePixelsPerSec(backBmp);
//     DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
//         "Million Vline pixels per sec = %.2f", score);
//     END_TEST;
// 
//     // Line draw
//     ClearBitmap(backBmp, g_colourBlack);
//     score = CalcMillionLinePixelsPerSec(backBmp);
//     DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
//         "Million line pixels per sec = %.2f", score);
//     END_TEST;
// 
    // Rect fill
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcBillionRectFillPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, 
        "Rectfill billion pixels per sec = %.2f", score);
    END_TEST;

//     // Text render
//     ClearBitmap(backBmp, g_colourBlack);
//     score = CalcMillionCharsPerSec(backBmp, font);
//     DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, 
//         "Million chars per sec = %.2f", score);
//     END_TEST;
// 
//     // Text render AA
//     ClearBitmap(backBmp, g_colourBlack);
//     score = CalcMillionAaCharsPerSec(backBmp, fontAa);
//     DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, 
//         "Million Anti-aliased chars per sec = %.2f", score);
//     END_TEST;
// 
//     // Polygon fill
//     ClearBitmap(backBmp, g_colourBlack);
//     score = CalcBillionPolyFillPixelsPerSec(backBmp);
//     DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, 
//         "Polyfill billion pixels per sec = %.2f", score);
//     END_TEST;

    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
        "Press ESC to quit");

    MasterGlyph *mg = fontAa->masterGlyphs['g'];
    for (int y = 0; y < mg->m_height; y++)
    {
        for (int x = 0; x < mg->m_width; x++)
        {
            bool pix = mg->GetPix(x, y);
            if (pix)
                PutPix(g_window->bmp, x + 400, y, g_colourWhite);
        }
    }

    UpdateWin();
    while (!g_window->windowClosed && !g_input.keyDowns[KEY_ESC])
    {
		InputManagerAdvance();
        DfSleepMillisec(100);
    }
}

#include "df_time.h"
#include "df_input.h"
#include "df_polygon.h"
#include "df_font.h"
#include "df_font_aa.h"
#include "df_window.h"

#include <stdio.h>
#include <string.h>

#include "df_master_glyph.h"


char *g_testFmtString = NULL;
unsigned g_iterations;
double g_duration;


inline unsigned GetRand()
{
    static unsigned r = 0x12345671;
    return r = ((r * 214013 + 2531011) >> 16) & 0xffff;
}


double CalcMillionPixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 40;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 128;
        PutPixUnclipped(bmp, r & 255, (r >> 8) & 255, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million putpix per sec";
    double numPixels = (double)g_iterations;
    return (numPixels / g_duration) / 1e6;
}


double CalcMillionHLinePixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 20;
    double startTime = GetRealTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < g_iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 255;
        HLine(bmp, r & 255, (r >> 8) & 255, lineLen, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million hline pixels per sec";
    double numPixels = (double)g_iterations * lineLen;
    return (numPixels / g_duration) / 1e6;
}


double CalcMillionVLinePixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 9;
    double startTime = GetRealTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < g_iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 255;
        VLine(bmp, r & 255, (r >> 8) & 255, lineLen, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million vline pixels per sec";
    double numPixels = (double)g_iterations * lineLen;
    return (numPixels / g_duration) / 1e6;
}


double CalcMillionLinePixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 300;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; ++i)
    {
        DrawLine(bmp, 0, 0, 20,  300, g_colourWhite);   // 320 long, nice slope
        DrawLine(bmp, 0, 0, 300, 20, g_colourWhite);
        DrawLine(bmp, 0, 0, 1,   15, g_colourWhite);    // 16 long, nice slope
        DrawLine(bmp, 0, 0, 15,  1, g_colourWhite);
        DrawLine(bmp, 0, 0, 150, 151, g_colourWhite);   // 151 long, annoying slope
        DrawLine(bmp, 0, 0, 151, 150, g_colourWhite);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million line pixels per sec";
    double numPixels = (320 + 16 + 151) * 2 * g_iterations;
    return (numPixels / g_duration) / 1e6;
}


double CalcBillionRectFillPixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 30;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++)
        RectFill(bmp, 0, 0, 300, 300, g_colourWhite);
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Billion rect fill pixels per sec";
    double numPixels = 300.0 * 300.0 * (double)g_iterations;
    return (numPixels / g_duration) / 1e9;
}


double CalcMillionCharsPerSec(DfBitmap *bmp, DfFont *font)
{
    static char const *str = "Here's some interesting text []£# !";
    g_iterations = 1000 * 500;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++) {
        int x = GetRand() % bmp->width;
        int y = i % bmp->height;
        DrawTextSimple(font, g_colourWhite, bmp, x, y, str);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million DrawText chars per sec";
    double numChars = g_iterations * (double)strlen(str);
    return (numChars / g_duration) / 1e6;
}


double CalcMillionAaCharsPerSec(DfBitmap *bmp, DfFontAa *font)
{
    static char const *str = "Here's some interesting text []£# !";
    g_iterations = 1000 * 100;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++) 
    {
        int x = GetRand() % bmp->width;
        int y = i % bmp->height;
        DrawTextSimpleAa(font, Colour(255, 255, 255, 60), bmp, x, y, 10, str);
    }
    double endTime = GetRealTime();

//    RectFill(bmp, 10, 10, 500, font->charHeight, g_colourBlack);
    DrawTextSimpleAa(font, Colour(255,255,255,255), bmp, 10, 10, 10, str);

    g_duration = endTime - startTime;
    g_testFmtString = "Million DrawText AA chars per sec";
    double numChars = g_iterations * (double)strlen(str);
    return (numChars / g_duration) / 1e6;
}


#define DRAW_POLYGON(pointList, Color, x, y)              \
    Polygon.numPoints = sizeof(pointList) / sizeof(Point);   \
    Polygon.points = pointList;                         \
    FillConvexPolygon(bmp, &Polygon, g_colourWhite, x, y);


double CalcBillionPolyFillPixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 4;
    double startTime = GetRealTime();
    for (unsigned k = 0; k < g_iterations; k++)
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
    
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Billion polygon fill pixels per sec";
    double numPixels = 87000.0 * (double)g_iterations;
    return (numPixels / g_duration) / 1e9;
}


double CalcWindowUpdatesPerSecond()
{
    g_iterations = 500;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++)
    {
        UpdateWin();
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Thousands blits backbuf to screen per sec";
    return (g_iterations / g_duration) / 1e3;
}


#define END_TEST \
    QuickBlit(g_window->bmp, 600, textY, backBmp); \
    UpdateWin(); \
    InputManagerAdvance(); \
    if (g_window->windowClosed || g_input.keyDowns[KEY_ESC]) return; \
    BitmapClear(backBmp, g_colourBlack); \
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, "%-42s %6.2f %5.2f %8u", g_testFmtString, score, g_duration, g_iterations); \
    fprintf(file, "%-42s %6.2f %6.2f %8u\n", g_testFmtString, score, g_duration, g_iterations); \
    textY += yInc;


void BenchmarkMain()
{
    FILE *file = fopen("report.txt", "w");

	CreateWin(1024, 768, WT_WINDOWED, "Benchmark");
    BitmapClear(g_window->bmp, g_colourBlack);

    DfFont *font = FontCreate("Courier New", 10, 4);
    DfFontAa *fontAa = FontAaCreate("Lucida Console", 4);

    DfBitmap *backBmp = BitmapCreate(1920, 1200);
    BitmapClear(backBmp, g_colourBlack);

    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, 10, "%-42s %6s %5s %8s", "", "Result", "Time", "Iterations");
    
    int textY = 30;
    int yInc = font->charHeight * 4;
    double score;

    // Put pixel
    score = CalcMillionPixelsPerSec(backBmp);
    END_TEST;

    // HLine draw
    score = CalcMillionHLinePixelsPerSec(backBmp);
    END_TEST;

    // VLine draw
    score = CalcMillionVLinePixelsPerSec(backBmp);
    END_TEST;

    // Line draw
    score = CalcMillionLinePixelsPerSec(backBmp);
    END_TEST;

    // Rect fill
    score = CalcBillionRectFillPixelsPerSec(backBmp);
    END_TEST;

    // Text render
    score = CalcMillionCharsPerSec(backBmp, font);
    END_TEST;

    // Text render AA
    score = CalcMillionAaCharsPerSec(backBmp, fontAa);
    END_TEST;

    SleepMillisec(1000);

    // Polygon fill
    score = CalcBillionPolyFillPixelsPerSec(backBmp);
    END_TEST;

    // Window updates
    score = CalcWindowUpdatesPerSecond();
    END_TEST;

    textY += yInc;
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
        "Press ESC to quit");

    UpdateWin();
    while (!g_window->windowClosed && !g_input.keyDowns[KEY_ESC])
    {
		InputManagerAdvance();
        SleepMillisec(100);
    }
}

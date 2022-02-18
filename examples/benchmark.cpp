#include "df_bmp.h"
#include "df_time.h"
#include "df_polygon.h"
#include "df_font.h"
#include "df_font_aa.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include <stdio.h>
#include <string.h>

#include "df_master_glyph.h"


const char *g_testFmtString = NULL;
unsigned g_iterations;
double g_duration;


inline unsigned GetRand()
{
    static unsigned r = 0x12345671;
    return r = ((r * 214013 + 2531011) >> 16) & 0xffff;
}


double CalcMillionPixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 30;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 128;
		unsigned x = r & 255;
		unsigned y = (r >> 8) & 255;
        PutPixUnclipped(bmp, x, y, c);
        PutPixUnclipped(bmp, x+1, y, c);
        PutPixUnclipped(bmp, x, y+1, c);
        PutPixUnclipped(bmp, x+1, y+1, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million unclipped putpix per sec";
	g_iterations *= 4;
    double numPixels = (double)g_iterations;
    return (numPixels / g_duration) / 1e6;
}


double CalcMillionHLinePixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 10;
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
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255, lineLen, c);
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255 + 1, lineLen, c);
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255 + 2, lineLen, c);
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255 + 3, lineLen, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million hline pixels per sec";
	g_iterations *= 4;
    double numPixels = (double)g_iterations * lineLen;
    return (numPixels / g_duration) / 1e6;
}


double CalcMillionHLineAlphaPixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 4;
    double startTime = GetRealTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < g_iterations; i++)
    {
        unsigned r = GetRand();
        DfColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 128;
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255, lineLen, c);
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255 + 1, lineLen, c);
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255 + 2, lineLen, c);
        HLineUnclipped(bmp, r & 255, (r >> 8) & 255 + 3, lineLen, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
	g_iterations *= 4;
    g_testFmtString = "Million hline alpha pixels per sec";
    double numPixels = (double)g_iterations * lineLen;
    return (numPixels / g_duration) / 1e6;
}


double CalcMillionVLinePixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 1000 * 5;
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
        unsigned x = r & 255;
        unsigned y = (r >> 8) & 255;
        VLine(bmp, x, y, lineLen, c);
        VLine(bmp, x+1, y, lineLen, c);
        VLine(bmp, x, y+1, lineLen, c);
        VLine(bmp, x+1, y+1, lineLen, c);
    }
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Million vline pixels per sec";
    g_iterations *= 4;
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
    double numPixels = (double)(320 + 16 + 151) * 2 * g_iterations;
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


double CalcBillionBlitPixelsPerSec(DfBitmap *bmp)
{
    DfBitmap *dog = LoadBmp("../../marlieses_dog.bmp");
    if (!dog) 
        dog = LoadBmp("marlieses_dog.bmp");
    ReleaseAssert(dog, "Couldn't load marlieses_dog.bmp");
    g_iterations = 1000 * 30;
    double startTime = GetRealTime();
    for (unsigned i = 0; i < g_iterations; i++)
        QuickBlit(bmp, 0, 0, dog);
    double endTime = GetRealTime();
    g_duration = endTime - startTime;
    g_testFmtString = "Billion blit pixels per sec";
    double numPixels = (double)dog->width * dog->height * (double)g_iterations;
    BitmapDelete(dog);
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


double CalcBillionPolyFillPixelsPerSec(DfBitmap *bmp)
{
    g_iterations = 1000 * 4;
    double startTime = GetRealTime();
    for (unsigned k = 0; k < g_iterations; k++)
    {
        static PolyVertList ScreenRectangle = { 4, {{340,10},{380,10},{380,200},{340,200}} };
        static PolyVertList Hexagon = { 6, {{190,250},{100,210},{10,250},{10,350},{100,390},{190,350}} };
        static PolyVertList Triangle1 = { 3, {{30,0},{15,20},{0,0}} };
        static PolyVertList Triangle2 = { 3, {{30,20},{15,0},{0,20}} };
        static PolyVertList Triangle3 = { 3, {{0,20},{20,10},{0,0}} };
        static PolyVertList Triangle4 = { 3, {{20,20},{20,0},{0,10}} };

        FillConvexPolygon(bmp, &ScreenRectangle, g_colourWhite, 0, 1);

        // Draw adjacent triangles across the top half of the screen.
        for (int j = 0; j <= 80; j += 20) {
            for (int i = 0; i < 290; i += 30) {
                FillConvexPolygon(bmp, &Triangle1, g_colourWhite, i, j);
                FillConvexPolygon(bmp, &Triangle2, g_colourWhite, i + 15, j);
            }
        }

        // Draw adjacent triangles across the bottom half of the screen.
        for (int j = 100; j <= 170; j += 20) {
            // Do a row of pointing-right triangles.
            for (int i = 0; i < 290; i += 20) {
                FillConvexPolygon(bmp, &Triangle3, g_colourWhite, i, j);
            }
            // Do a row of pointing-left triangles halfway between one row
            // of pointing-right triangles and the next, to fit between.
            for (int i = 0; i < 290; i += 20) {
                FillConvexPolygon(bmp, &Triangle4, g_colourWhite, i, j + 10);
            }
        }

        FillConvexPolygon(bmp, &Hexagon, g_colourWhite, 0, 0);
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
    InputPoll(); \
    if (g_window->windowClosed || g_window->input.keyDowns[KEY_ESC]) return; \
    BitmapClear(backBmp, g_colourBlack); \
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY, "%-42s %7.2f %5.2f %9u", g_testFmtString, score, g_duration, g_iterations); \
    fprintf(file, "%-42s %7.2f %6.2f %9u\n", g_testFmtString, score, g_duration, g_iterations); \
    textY += yInc;


void BenchmarkMain()
{
    FILE *file = fopen("report.txt", "w");
    ReleaseAssert(file, "Couldn't open report.txt");

	CreateWin(1024, 768, WT_WINDOWED, "Benchmark");
    BitmapClear(g_window->bmp, g_colourBlack);

    DfFont *font = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));
    DfFontAa *fontAa = FontAaCreate("Lucida Console", 4);

    DfBitmap *backBmp = BitmapCreate(1920, 1200);
    BitmapClear(backBmp, g_colourBlack);

    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, 10, "%-42s %7s %5s %9s", "", "Result", "Time", "Iterations");
    
    int textY = 30;
    int yInc = font->charHeight * 4;
    double score;

    // Put pixel
    score = CalcMillionPixelsPerSec(backBmp);
    END_TEST;

    // HLine draw
    score = CalcMillionHLinePixelsPerSec(backBmp);
    END_TEST;

    // HLine alpha draw
    score = CalcMillionHLineAlphaPixelsPerSec(backBmp);
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

    // Blit
    score = CalcBillionBlitPixelsPerSec(backBmp);
    END_TEST;

    // Text render
    score = CalcMillionCharsPerSec(backBmp, font);
    END_TEST;

    // Text render AA
    score = CalcMillionAaCharsPerSec(backBmp, fontAa);
    END_TEST;

    // Polygon fill
    score = CalcBillionPolyFillPixelsPerSec(backBmp);
    END_TEST;

    // Window updates
    score = CalcWindowUpdatesPerSecond();
    END_TEST;

    fclose(file);

    textY += yInc;
    DrawTextLeft(font, g_colourWhite, g_window->bmp, 10, textY,
        "Press ESC to quit");

    UpdateWin();
    while (!g_window->windowClosed && !g_window->input.keyDowns[KEY_ESC])
    {
		InputPoll();
        SleepMillisec(100);
    }
}

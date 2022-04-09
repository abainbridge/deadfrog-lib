#include "df_time.h"
#include "df_polygon.h"
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include <stdint.h>
#include <string.h>


inline unsigned GetRand()
{
    static unsigned r = 0x12345671;
    return r = ((r * 214013 + 2531011) >> 16) & 0xffff;
}


void Check(DfBitmap *bmp)
{
    for (int y = 0; y < bmp->clipTop; y++)
        for (int x = 0; x < bmp->width; x++)
            ReleaseAssert(GetPixUnclipped(bmp, x, y).c == g_colourBlack.c, "Top region not blank");
    
    for (int y = bmp->clipTop; y < bmp->clipBottom; y++)
    {
        for (int x = 0; x < bmp->clipLeft; x++)
            ReleaseAssert(GetPixUnclipped(bmp, x, y).c == g_colourBlack.c, "Left region not blank");
        for (int x = bmp->clipRight; x < bmp->width; x++)
            ReleaseAssert(GetPixUnclipped(bmp, x, y).c == g_colourBlack.c, "Right region not blank");
    }

    for (int y = bmp->clipBottom; y < bmp->height; y++)
        for (int x = 0; x < bmp->width; x++)
            ReleaseAssert(GetPixUnclipped(bmp, x, y).c == g_colourBlack.c, "Bottom region not blank");
}


void TestPutPix(DfBitmap *bmp)
{
    for (int y = 0; y < bmp->height; y++)
        for (int x = 0; x < bmp->width; x++)
            PutPix(bmp, x, y, g_colourWhite);

    Check(bmp);
}


void TestHLine(DfBitmap *bmp)
{
    for (int y = 0; y < bmp->height; y++)
    {
        for (unsigned i = 0; i < 1000; i++)
        {
            int x = (y * i + GetRand()) % bmp->width;
            int lineLen = GetRand() % 20;
            HLine(bmp, x, y, lineLen, g_colourWhite);
        }
    }

    Check(bmp);
}


void TestVLine(DfBitmap *bmp)
{
    for (int x = 0; x < bmp->width; x++)
    {
        for (unsigned i = 0; i < 1000; i++)
        {
            int y = (x * i + GetRand()) % bmp->width;
            int lineLen = GetRand() % 20;
            VLine(bmp, x, y, lineLen, g_colourWhite);
        }
    }

    Check(bmp);
}


void TestDrawLine(DfBitmap *bmp)
{
    for (int y = -1; y < bmp->height + 1; y++)
    {
        for (int x = -1; x < bmp->width + 1; x++)
        {
            for (int i = 0; i < 2; i++)
            {
                int x2 = x + GetRand() % 20 - 10;
                int y2 = y + GetRand() % 20 - 10;
                DrawLine(bmp, x, y, x2, y2, g_colourWhite);
            }
        }
    }

    Check(bmp);
}


void TestRectFill(DfBitmap *bmp)
{
    for (int y = -1; y < bmp->height + 1; y++)
    {
        int iterations = 30;
        for (unsigned i = 0; i < iterations; i++)
        {
            int x = GetRand() % bmp->width;
            int w = GetRand() % 20;
            int h = GetRand() % 20;
            RectFill(bmp, x, y, w, h, g_colourWhite);
        }
    }

    Check(bmp);
}


void TestDrawText(DfBitmap *bmp, DfFont *font)
{
    static char const *str = "Here's some interesting text []£# !";
    int iterations = 1000 * 500;
    for (unsigned i = 0; i < iterations; i++) {
        int x = GetRand() % bmp->width;
        int y = i % bmp->height;
        DrawTextSimple(font, g_colourWhite, bmp, x, y, str);
    }

    Check(bmp);
}


void TestFillConvexPolygon(DfBitmap *bmp)
{
    int iterations = 1;
    for (unsigned k = 0; k < iterations; k++)
    {
        static PolyVertList ScreenRectangle = { 4, { { 340, 10 }, { 380, 10 }, { 380, 200 }, { 340, 200 } } };
        static PolyVertList Hexagon = { 6, { { 190, 250 }, { 100, 210 }, { 10, 250 }, { 10, 350 }, { 100, 390 }, { 190, 350 } } };
        static PolyVertList Triangle1 = { 3, { { 30, 0 }, { 15, 20 }, { 0, 0 } } };
        static PolyVertList Triangle2 = { 3, { { 30, 20 }, { 15, 0 }, { 0, 20 } } };
        static PolyVertList Triangle3 = { 3, { { 0, 20 }, { 20, 10 }, { 0, 0 } } };
        static PolyVertList Triangle4 = { 3, { { 20, 20 }, { 20, 0 }, { 0, 10 } } };

        FillConvexPolygon(bmp, &ScreenRectangle, g_colourWhite, 0, 1);

        // Draw adjacent triangles across the top half of the screen
        int i, j;
        for (j = 0; j <= 80; j += 20) {
            for (i = 0; i < 290; i += 30) {
                FillConvexPolygon(bmp, &Triangle1, g_colourWhite, i, j);
                FillConvexPolygon(bmp, &Triangle2, g_colourWhite, i + 15, j);
            }
        }

        // Draw adjacent triangles across the bottom half of the screen
        for (j = 100; j <= 170; j += 20) {
            // Do a row of pointing-right triangles
            for (i = 0; i < 290; i += 20) {
                FillConvexPolygon(bmp, &Triangle3, g_colourWhite, i, j);
            }
            // Do a row of pointing-left triangles halfway between one row
            // of pointing-right triangles and the next, to fit between
            for (i = 0; i < 290; i += 20) {
                FillConvexPolygon(bmp, &Triangle4, g_colourWhite, i, j + 10);
            }
        }

        FillConvexPolygon(bmp, &Hexagon, g_colourWhite, 0, 0);
    }

    Check(bmp);
}


#define END_TEST \
    Blit(win->bmp, 600, textY, backBmp); \
    UpdateWin(win); \
    InputPoll(win); \
if (win->windowClosed || win->input.keyDowns[KEY_ESC]) return; \
    ClearClipRect(backBmp); \
    BitmapClear(backBmp, g_colourBlack); \
    SetClipRect(backBmp, 1, 1, backBmp->width - 2, backBmp->height - 2); \
    textY += yInc;


void TestMain()
{
    DfWindow *win = CreateWin(1024, 768, WT_WINDOWED_RESIZEABLE, "Test");
    BitmapClear(win->bmp, g_colourBlack);

    DfFont *font = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    DfBitmap *backBmp = BitmapCreate(1920, 1200);
    BitmapClear(backBmp, g_colourBlack);

    int textY = 30;
    int yInc = font->charHeight * 4;

    // PutPix
    TestPutPix(backBmp);
    END_TEST;

    // HLine draw
    TestHLine(backBmp);
    END_TEST;

    // VLine draw
    TestVLine(backBmp);
    END_TEST;

    // Line draw
    TestDrawLine(backBmp);
    END_TEST;

    // Rect fill
    TestRectFill(backBmp);
    END_TEST;

    // Text render
    TestDrawText(backBmp, font);
    END_TEST;

    // Polygon fill
    TestFillConvexPolygon(backBmp);
    END_TEST;

    textY += yInc;
    DrawTextLeft(font, g_colourWhite, win->bmp, 10, textY,
        "Press ESC to quit");

    UpdateWin(win);
    while (!win->windowClosed && !win->input.keyDowns[KEY_ESC])
    {
        InputPoll(win);
        SleepMillisec(100);
    }
}

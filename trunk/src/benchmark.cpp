#include <string.h>
#include <windows.h>

#include "lib/gfx/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/window_manager.h"


double CalcBillionPixelsPerSec(BitmapRGBA *bmp)
{
    unsigned iterations = 1000 * 1000 * 100;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        PlotUnclipped(bmp, 10, 10, g_colourWhite);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations;
    return (numPixels / duration) / 1e9;
}


double CalcBillionRectFillPixelsPerSec(BitmapRGBA *bmp)
{
    unsigned iterations = 1000 * 300;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        RectFill(bmp, 10, 10, 100, 100, g_colourWhite);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = 100.0 * 100.0 * (double)iterations;
    return (numPixels / duration) / 1e9;
}


double CalcMillionLinePixelsPerSec(BitmapRGBA *bmp)
{
    unsigned iterations = 1000 * 600;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; ++i)
    {
        DrawLine(bmp, 300, 300, 320, 600, g_colourWhite);   // 320 long, nice slope
        DrawLine(bmp, 300, 300, 600, 320, g_colourWhite);
        DrawLine(bmp, 300, 300, 301, 315, g_colourWhite);   // 16 long, nice slope
        DrawLine(bmp, 300, 300, 315, 301, g_colourWhite);
        DrawLine(bmp, 300, 300, 450, 451, g_colourWhite);   // 151 long, annoying slope
        DrawLine(bmp, 300, 300, 451, 450, g_colourWhite);
    }
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (320 + 16 + 151) * 2 * iterations;
    return (numPixels / duration) / 1e6;
}


double CalcMillionCharsPerSec(BitmapRGBA *bmp, TextRenderer *font)
{
    static char const *str = "Here's some interesting text []£# !";
    unsigned iterations = 1000 * 100;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        DrawTextSimple(font, g_colourWhite, bmp, 10, 10, str);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numChars = iterations * (double)strlen(str);
    return (numChars / duration) / 1000000.0;
}


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE, LPSTR cmdLine, int)
{
	WindowData *win = CreateWin(100, 100, 1024, 768, true, "Benchmark");
    TextRenderer *font = CreateTextRenderer("Courier", 8);
    BitmapRGBA *backBmp = CreateBitmapRGBA(800, 600);

    InitialiseHighResTime();
    ClearBitmap(win->bmp, g_colourBlack);
    int textY = 10;
    double score;

    // Put pixel
    score = CalcBillionPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, win->bmp, 10, textY, 
        "Billion putpixels per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin(win);

    // Line draw
    score = CalcMillionLinePixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, win->bmp, 10, textY,
        "Million line pixels per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin(win);

    // Rect fill
    score = CalcBillionRectFillPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, win->bmp, 10, textY, 
        "Rectfill billion pixels per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin(win);

    // Text render
    score = CalcMillionCharsPerSec(backBmp, font);
    DrawTextLeft(font, g_colourWhite, win->bmp, 10, textY, 
        "Million chars per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin(win);

    DrawTextLeft(font, g_colourWhite, win->bmp, 10, textY,
        "Press ESC to quit");

    while (!win->windowClosed && !win->inputManager->keyDowns[KEY_ESC])
    {
        AdvanceWin(win);
    }
}

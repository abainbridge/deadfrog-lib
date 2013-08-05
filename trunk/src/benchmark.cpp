#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"

#include <string.h>
#include <windows.h>


double CalcBillionPixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 1000 * 100;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        PutPixUnclipped(backBuffer, 10, 10, g_colourWhite);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations;
    return (numPixels / duration) / 1e9;
}


double CalcBillionRectFillPixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 300;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        RectFill(backBuffer, 10, 10, 100, 100, g_colourWhite);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = 100.0 * 100.0 * (double)iterations;
    return (numPixels / duration) / 1e9;
}


double CalcMillionLinePixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 600;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; ++i)
    {
        DrawLine(backBuffer, 300, 300, 320, 600, g_colourWhite);   // 320 long, nice slope
        DrawLine(backBuffer, 300, 300, 600, 320, g_colourWhite);
        DrawLine(backBuffer, 300, 300, 301, 315, g_colourWhite);   // 16 long, nice slope
        DrawLine(backBuffer, 300, 300, 315, 301, g_colourWhite);
        DrawLine(backBuffer, 300, 300, 450, 451, g_colourWhite);   // 151 long, annoying slope
        DrawLine(backBuffer, 300, 300, 451, 450, g_colourWhite);
    }
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (320 + 16 + 151) * 2 * iterations;
    return (numPixels / duration) / 1e6;
}


double CalcMillionCharsPerSec(BitmapRGBA *backBuffer, TextRenderer *font)
{
    static char const *str = "Here's some interesting text []£# !";
    unsigned iterations = 1000 * 100;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        DrawTextSimple(font, g_colourWhite, backBuffer, 10, 10, str);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numChars = iterations * (double)strlen(str);
    return (numChars / duration) / 1000000.0;
}


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE, LPSTR cmdLine, int)
{
	CreateWin(100, 100, 1024, 768, true, "Benchmark");
    TextRenderer *font = CreateTextRenderer("Courier", 8);
    BitmapRGBA *backBmp = CreateBitmapRGBA(800, 600);

    ClearBitmap(g_window->backBuffer, g_colourBlack);
    int textY = 10;
    double score;

    // Put pixel
    score = CalcBillionPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY, 
        "Billion putpixels per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin();

    // Line draw
    score = CalcMillionLinePixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY,
        "Million line pixels per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin();

    // Rect fill
    score = CalcBillionRectFillPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY, 
        "Rectfill billion pixels per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin();

    // Text render
    score = CalcMillionCharsPerSec(backBmp, font);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY, 
        "Million chars per sec = %.2f", score);
    textY += font->charHeight;
    AdvanceWin();

    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY,
        "Press ESC to quit");

    while (!g_window->windowClosed && !g_inputManager.keyDowns[KEY_ESC])
    {
        AdvanceWin();
        Sleep(100);
    }
}

#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"

#include <string.h>
#include <windows.h>


inline unsigned GetRand()
{
    static unsigned r = 0x12345671;
    return r = ((r * 214013 + 2531011) >> 16) & 0xffff;
}


double CalcMillionPixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 1000 * 10;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
    {
        unsigned r = GetRand();
        RGBAColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 128;
        PutPixUnclipped(backBuffer, r & 255, (r >> 8) & 255, c);
    }
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations;
    return (numPixels / duration) / 1e6;
}


double CalcMillionHLinePixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 1000 * 10;
    double startTime = GetHighResTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < iterations; i++)
    {
        unsigned r = GetRand();
        RGBAColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 255;
        HLine(backBuffer, r & 255, (r >> 8) & 255, lineLen, c);
    }
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations * lineLen;
    return (numPixels / duration) / 1e6;
}


double CalcMillionVLinePixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 1000 * 10;
    double startTime = GetHighResTime();
    unsigned lineLen = 20;
    for (unsigned i = 0; i < iterations; i++)
    {
        unsigned r = GetRand();
        RGBAColour c;
        c.r = r & 0xff;
        c.g = r & 0xff;
        c.b = r & 0xff;
        c.a = 255;
        VLine(backBuffer, r & 255, (r >> 8) & 255, lineLen, c);
    }
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (double)iterations * lineLen;
    return (numPixels / duration) / 1e6;
}


double CalcBillionRectFillPixelsPerSec(BitmapRGBA *backBuffer)
{
    unsigned iterations = 1000 * 100;
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
        DrawLine(backBuffer, 0, 0, 20,  300, g_colourWhite);   // 320 long, nice slope
        DrawLine(backBuffer, 0, 0, 300, 20, g_colourWhite);
        DrawLine(backBuffer, 0, 0, 1,   15, g_colourWhite);    // 16 long, nice slope
        DrawLine(backBuffer, 0, 0, 15,  1, g_colourWhite);
        DrawLine(backBuffer, 0, 0, 150, 151, g_colourWhite);   // 151 long, annoying slope
        DrawLine(backBuffer, 0, 0, 151, 150, g_colourWhite);
    }
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numPixels = (320 + 16 + 151) * 2 * iterations;
    return (numPixels / duration) / 1e6;
}


double CalcMillionCharsPerSec(BitmapRGBA *backBuffer, TextRenderer *font)
{
    static char const *str = "Here's some interesting text []�# !";
    unsigned iterations = 1000 * 100;
    double startTime = GetHighResTime();
    for (unsigned i = 0; i < iterations; i++)
        DrawTextSimple(font, g_colourWhite, backBuffer, 10, 10, str);
    double endTime = GetHighResTime();
    double duration = endTime - startTime;
    double numChars = iterations * (double)strlen(str);
    return (numChars / duration) / 1000000.0;
}


#define END_TEST \
    textY += yInc; \
    QuickBlit(g_window->backBuffer, 400, textY, backBmp); \
    AdvanceWin(); \
    if (g_window->windowClosed || g_inputManager.keyDowns[KEY_ESC]) return 0; \
    ClearBitmap(backBmp, g_colourBlack);


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE, LPSTR cmdLine, int)
{
	CreateWin(100, 100, 1024, 768, true, "Benchmark");
    TextRenderer *font = CreateTextRenderer("Courier", 8);
    BitmapRGBA *backBmp = CreateBitmapRGBA(800, 600);

    ClearBitmap(g_window->backBuffer, g_colourBlack);
    int textY = 10;
    int yInc = font->charHeight * 3;
    double score;

    // Put pixel
    score = CalcMillionPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY, 
        "Million putpixels per sec = %.1f", score);
    END_TEST;

    // HLine draw
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcMillionHLinePixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY,
        "Million Hline pixels per sec = %.2f", score);
    END_TEST;

    // VLine draw
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcMillionVLinePixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY,
        "Million Vline pixels per sec = %.2f", score);
    END_TEST;

    // Line draw
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcMillionLinePixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY,
        "Million line pixels per sec = %.2f", score);
    END_TEST;

    // Rect fill
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcBillionRectFillPixelsPerSec(backBmp);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY, 
        "Rectfill billion pixels per sec = %.2f", score);
    END_TEST;

    // Text render
    ClearBitmap(backBmp, g_colourBlack);
    score = CalcMillionCharsPerSec(backBmp, font);
    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY, 
        "Million chars per sec = %.2f", score);
    END_TEST;

    DrawTextLeft(font, g_colourWhite, g_window->backBuffer, 10, textY,
        "Press ESC to quit");

    while (!g_window->windowClosed && !g_inputManager.keyDowns[KEY_ESC])
    {
        AdvanceWin();
        Sleep(100);
    }

    return 0;
}

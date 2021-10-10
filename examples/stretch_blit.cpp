// Deadfrog includes
#include "df_bmp.h"
#include "df_font.h"
#include "df_time.h"
#include "df_window.h"
#include "fonts/df_mono.h"

// Standard includes
#include <math.h>


void StretchBlitMain() {
    CreateWin(1024, 768, WT_WINDOWED, "Stretch Blit Example");

    g_defaultFont = LoadFontFromMemory(deadfrog_mono_7x13, sizeof(deadfrog_mono_7x13));

    DfBitmap *dogBmp = LoadBmp("marlieses_dog.bmp");
    if (!dogBmp) {
        dogBmp = LoadBmp("../../marlieses_dog.bmp");
    }
    ReleaseAssert(dogBmp, "Couldn't load marlieses_dog.bmp");

    // Animate the stretch blit.
    while (!g_input.keyDowns[KEY_SPACE]) {
        InputPoll();
        if (g_window->windowClosed || g_input.keys[KEY_ESC])
            return;

        BitmapClear(g_window->bmp, g_colourBlack);

        double time = GetRealTime();
        float scale = 0.2f + powf(fabs(sin(time * 1.9) * 1.2), 2.0);
        int targetWidth = dogBmp->width * scale;
        int targetHeight = dogBmp->width * scale;

        int x = g_window->bmp->width * (0.5f + 0.7 * sin(time * 2.5)) - targetHeight / 2.0f;
        int y = g_window->bmp->height * (0.5f + 0.7 * sin(time * 1.3)) - targetWidth / 2.0f;
        StretchBlit(g_window->bmp, x, y, targetWidth, targetHeight, dogBmp);

        UpdateWin();
        WaitVsync();
    }

    // Benchmark the stretch blit.
    while (!g_window->windowClosed && !g_input.keys[KEY_ESC]) {
        InputPoll();

        BitmapClear(g_window->bmp, g_colourBlack);

        double time1 = GetRealTime();

        int targetWidth = dogBmp->width * 0.1;
        int targetHeight = dogBmp->width * 0.1;
        StretchBlit(g_window->bmp, 10, 15, targetWidth, targetHeight, dogBmp);

        double time2 = GetRealTime();

        targetWidth = dogBmp->width * 0.7;
        targetHeight = dogBmp->width * 0.7;
        StretchBlit(g_window->bmp, 100, 15, targetWidth, targetHeight, dogBmp);

        double time3 = GetRealTime();

        targetWidth = dogBmp->width * 2.0;
        targetHeight = dogBmp->width * 2.0;
        StretchBlit(g_window->bmp, 400, 15, targetWidth, targetHeight, dogBmp);

        double time4 = GetRealTime();

        DrawTextLeft(g_defaultFont, g_colourWhite, g_window->bmp, 10, 0, "%.2fms", (time2 - time1) * 1000.0f);
        DrawTextLeft(g_defaultFont, g_colourWhite, g_window->bmp, 100, 0, "%.2fms", (time3 - time2) * 1000.0f);
        DrawTextLeft(g_defaultFont, g_colourWhite, g_window->bmp, 400, 0, "%.2fms", (time4 - time3) * 1000.0f);

        UpdateWin();
        WaitVsync();
    }
}

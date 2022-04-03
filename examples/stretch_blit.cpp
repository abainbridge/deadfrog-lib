// Deadfrog includes
#include "df_bmp.h"
#include "df_font.h"
#include "df_time.h"
#include "df_window.h"
#include "fonts/df_mono.h"

// Standard includes
#include <math.h>


void StretchBlitMain() {
    DfWindow *win = CreateWin(1024, 768, WD_WINDOWED_RESIZEABLE, "Stretch Blit Example");

    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    DfBitmap *dogBmp = LoadBmp("marlieses_dog.bmp");
    if (!dogBmp) {
        dogBmp = LoadBmp("../../marlieses_dog.bmp");
    }
    ReleaseAssert(dogBmp, "Couldn't load marlieses_dog.bmp");

    // Animate the stretch blit.
    while (!win->input.keyDowns[KEY_SPACE]) {
        InputPoll(win);
        if (win->windowClosed || win->input.keys[KEY_ESC])
            return;

        BitmapClear(win->bmp, g_colourBlack);

        double time = GetRealTime();
        float scale = 0.2f + powf(fabs(sin(time * 1.9) * 1.2), 2.0);
        int targetWidth = dogBmp->width * scale;
        int targetHeight = dogBmp->width * scale;

        int x = win->bmp->width * (0.5f + 0.7 * sin(time * 2.5)) - targetHeight / 2.0f;
        int y = win->bmp->height * (0.5f + 0.7 * sin(time * 1.3)) - targetWidth / 2.0f;
        StretchBlit(win->bmp, x, y, targetWidth, targetHeight, dogBmp);

        UpdateWin(win);
        WaitVsync();
    }

    // Benchmark the stretch blit.
    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        InputPoll(win);

        BitmapClear(win->bmp, g_colourBlack);

        double time1 = GetRealTime();

        int targetWidth = dogBmp->width * 0.1;
        int targetHeight = dogBmp->width * 0.1;
        StretchBlit(win->bmp, 10, 15, targetWidth, targetHeight, dogBmp);

        double time2 = GetRealTime();

        targetWidth = dogBmp->width * 0.7;
        targetHeight = dogBmp->width * 0.7;
        StretchBlit(win->bmp, 100, 15, targetWidth, targetHeight, dogBmp);

        double time3 = GetRealTime();

        targetWidth = dogBmp->width * 2.0;
        targetHeight = dogBmp->width * 2.0;
        StretchBlit(win->bmp, 400, 15, targetWidth, targetHeight, dogBmp);

        double time4 = GetRealTime();

        DrawTextLeft(g_defaultFont, g_colourWhite, win->bmp, 10, 0, "%.2fms", (time2 - time1) * 1000.0f);
        DrawTextLeft(g_defaultFont, g_colourWhite, win->bmp, 100, 0, "%.2fms", (time3 - time2) * 1000.0f);
        DrawTextLeft(g_defaultFont, g_colourWhite, win->bmp, 400, 0, "%.2fms", (time4 - time3) * 1000.0f);

        UpdateWin(win);
        WaitVsync();
    }
}

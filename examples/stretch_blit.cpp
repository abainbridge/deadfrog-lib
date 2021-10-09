// Deadfrog includes
#include "df_bmp.h"
#include "df_time.h"
#include "df_window.h"

// Standard includes
#include <math.h>


void StretchBlitMain() {
    CreateWin(1024, 768, WT_WINDOWED, "Stretch Blit Example");

    DfBitmap *dogBmp = LoadBmp("marlieses_dog.bmp");
    if (!dogBmp) {
        dogBmp = LoadBmp("../../marlieses_dog.bmp");
    }
    ReleaseAssert(dogBmp, "Couldn't load marlieses_dog.bmp");

    while (!g_window->windowClosed && !g_input.keys[KEY_ESC]) {
        InputPoll();

        BitmapClear(g_window->bmp, g_colourBlack);

        double time = GetRealTime();
        float scale = 0.1f + fabs(sin(time * 1.9) * 1.2);
        int targetWidth = dogBmp->width * scale;
        int targetHeight = dogBmp->width * scale;

        int x = g_window->bmp->width * (0.5f + 0.2 * sin(time * 1.5)) - targetHeight / 2.0f;
        int y = g_window->bmp->height * (0.5f + 0.2 * sin(time * 1.3)) - targetWidth / 2.0f;
        StretchBlit(g_window->bmp, x, y, targetWidth, targetHeight, dogBmp);

        UpdateWin();
        WaitVsync();
    }
}

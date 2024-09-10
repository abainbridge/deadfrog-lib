// 2D fake gravity simulation. Simulates gravity with a 1/r fall-off over
// distance instead of 1/r^2

#include "df_font.h"
#include "fonts/df_prop.h"
#include "df_window.h"
#include <immintrin.h>
#include <stdlib.h>

enum { NUM_PARTICLES = 20000 };
__declspec(align(32)) float g_manyX[NUM_PARTICLES];
__declspec(align(32)) float g_manyY[NUM_PARTICLES];
__declspec(align(32)) float g_manyVx[NUM_PARTICLES];
__declspec(align(32)) float g_manyVy[NUM_PARTICLES];

void GravityMain() {
    g_window = CreateWin(1150, 1150, WT_WINDOWED_RESIZEABLE, "Gravity Example");
    g_defaultFont = LoadFontFromMemory(df_prop_7x13, sizeof(df_prop_7x13));

    for (unsigned i = 0; i < NUM_PARTICLES; i++) {
        g_manyX[i] = (float)rand() / (float)RAND_MAX - 0.5f;
        g_manyY[i] = (float)rand() / (float)RAND_MAX - 0.5f;
        g_manyVx[i] = -g_manyY[i] * 0.02f;
        g_manyVy[i] = g_manyX[i] * 0.02f;
    }

    // Maybe we can make all this use 256-bit registers by implementing this:
    // https://stackoverflow.com/questions/75628394/how-to-get-mm256-rcp-pd-in-avx2

    __m128 _gravityScale = _mm_set_ps1(1e-8f);
    while (!g_window->windowClosed && !g_window->input.keys[KEY_ESC]) {
        BitmapClear(g_window->bmp, g_colourBlack);
        InputPoll(g_window);

        float zoom = g_window->bmp->height * 0.4;
        int centreX = g_window->bmp->width / 2;
        int centreY = g_window->bmp->height / 2;
        for (unsigned i = 0; i < NUM_PARTICLES; i++) {
            __m128 _px = _mm_set_ps1(g_manyX[i]);
            __m128 _py = _mm_set_ps1(g_manyY[i]);
            __m128 _accumulatedImpulseX = _mm_setzero_ps();
            __m128 _accumulatedImpulseY = _mm_setzero_ps();

            unsigned lastFastI = i & ~3;
            for (unsigned j = 0; j < lastFastI; j += 4) {
                __m128 _qx = _mm_load_ps(g_manyX + j);
                __m128 _qy = _mm_load_ps(g_manyY + j);

                __m128 _deltaX = _mm_sub_ps(_qx, _px); // float deltaX = g_manyX[j] - g_manyX[i];
                __m128 _deltaY = _mm_sub_ps(_qy, _py); // float deltaY = g_manyY[j] - g_manyY[i];

                // float distSquared = (deltaX * deltaX + deltaY * deltaY);
                __m128 _distSquared = _mm_mul_ps(_deltaX, _deltaX);
                _distSquared = _mm_fmadd_ps(_deltaY, _deltaY, _distSquared);

                // float force = 1e-7 / distSquared;
                __m128 _force = _mm_rcp_ps(_distSquared);
                _force = _mm_mul_ps(_force, _gravityScale);

                __m128 _impulseX = _mm_mul_ps(_force, _deltaX); // float impulseX = force * deltaX;
                __m128 _impulseY = _mm_mul_ps(_force, _deltaY); // float impulseY = force * deltaY;

                // accumulatedImpulseX += impulseX;
                _accumulatedImpulseX = _mm_add_ps(_accumulatedImpulseX, _impulseX);

                // accumulatedImpulseY += impulseY;
                _accumulatedImpulseY = _mm_add_ps(_accumulatedImpulseY, _impulseY);

                // g_manyVx[j] -= impulseX;
                __m128 _vxj = _mm_load_ps(g_manyVx + j);
                _vxj = _mm_sub_ps(_vxj, _impulseX);
                _mm_store_ps(g_manyVx + j, _vxj);

                // g_manyVy[j] -= impulseY;
                __m128 _vyj = _mm_load_ps(g_manyVy + j);
                _vyj = _mm_sub_ps(_vyj, _impulseY);
                _mm_store_ps(g_manyVy + j, _vyj);
            }

            __declspec(align(16)) float accumulatedImpulsesX[4];
            __declspec(align(16)) float accumulatedImpulsesY[4];
            _mm_store_ps(accumulatedImpulsesX, _accumulatedImpulseX);
            _mm_store_ps(accumulatedImpulsesY, _accumulatedImpulseY);
            for (unsigned j = 0; j < 4; j++) {
                g_manyVx[i] += accumulatedImpulsesX[j];
                g_manyVy[i] += accumulatedImpulsesY[j];
            }

            float accumulatedImpulseX = 0.0f;
            float accumulatedImpulseY = 0.0f;
            for (unsigned j = lastFastI; j < i; j++) {
                //            for (unsigned j = 0; j < i; j++) {
                float deltaX = g_manyX[j] - g_manyX[i];
                float deltaY = g_manyY[j] - g_manyY[i];
                float distSquared = (deltaX * deltaX + deltaY * deltaY);
                float force = 1e-7 / distSquared;
                float impulseX = force * deltaX;
                float impulseY = force * deltaY;
                accumulatedImpulseX += impulseX;
                accumulatedImpulseY += impulseY;
                g_manyVx[j] -= impulseX;
                g_manyVy[j] -= impulseY;
            }

            g_manyVx[i] += accumulatedImpulseX;
            g_manyVy[i] += accumulatedImpulseY;
            g_manyX[i] += g_manyVx[i];
            g_manyY[i] += g_manyVy[i];
            int sx = g_manyX[i] * zoom + centreX;
            int sy = g_manyY[i] * zoom + centreY;
            PutPix(g_window->bmp, sx, sy, g_colourWhite);
        }

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        UpdateWin(g_window);
        //        WaitVsync();
    }
}

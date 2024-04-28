// 2D fake gravity simulation. Simulates gravity with a 1/r fall-off over
// distance instead of 1/r^2

#include "df_font.h"
#include "fonts/df_prop.h"
#include "df_window.h"
#include <stdlib.h>

enum { NUM_PARTICLES = 4000 };
struct Partical { float x, y, vx, vy; };
Partical g_manyParticles[NUM_PARTICLES];

void GravityMain() {
    g_window = CreateWin(800, 600, WT_WINDOWED_RESIZEABLE, "Gravity Example");
    g_defaultFont = LoadFontFromMemory(df_prop_7x13, sizeof(df_prop_7x13));

    for (unsigned i = 0; i < NUM_PARTICLES; i++) {
        Partical* p = &g_manyParticles[i];
        p->x = (float)rand() / (float)RAND_MAX - 0.5f;
        p->y = (float)rand() / (float)RAND_MAX - 0.5f;
        p->vx = -p->y * 0.05f;
        p->vy = p->x * 0.05f;
    }

    while (!g_window->windowClosed && !g_window->input.keys[KEY_ESC]) {
        BitmapClear(g_window->bmp, g_colourBlack);
        InputPoll(g_window);

        float zoom = g_window->bmp->height * 0.4;
        int centreX = g_window->bmp->width / 2;
        int centreY = g_window->bmp->height / 2;
        for (unsigned i = 0; i < NUM_PARTICLES; i++) {
            Partical* p = &g_manyParticles[i];

            for (unsigned j = 0; j < i; j++) {
                Partical* q = &g_manyParticles[j];
                float deltaX = q->x - p->x;
                float deltaY = q->y - p->y;
                float distSquared = (deltaX * deltaX + deltaY * deltaY);
                float force = 1e-7 / distSquared;
                float impulseX = force * deltaX;
                float impulseY = force * deltaY;
                p->vx += impulseX;
                p->vy += impulseY;
                q->vx -= impulseX;
                q->vy -= impulseY;
            }

            p->x += p->vx;
            p->y += p->vy;
            int sx = p->x * zoom + centreX;
            int sy = p->y * zoom + centreY;
            PutPix(g_window->bmp, sx, sy, g_colourWhite);
        }

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        UpdateWin(g_window);
        WaitVsync();
    }
}

// This example is a re-implementation of the drawing effect from the
// classic "Beziers" screen saver from Windows 95.

#include "df_bitmap.h"
#include "df_window.h"
#include <stdlib.h>


struct BezierPoint {
    double x, y;
    double velX, velY;
};


void BezierPointInit(BezierPoint *p) {
    p->x = rand() % g_window->bmp->width;
    p->y = rand() % g_window->bmp->height;
    p->velX = (rand() / (double)RAND_MAX - 0.5) * 15.0;
    p->velY = (rand() / (double)RAND_MAX - 0.5) * 15.0;
}


void BezierPointAdvance(BezierPoint *p) {
    // Update X component of point.
    p->x += p->velX;
    if (p->x < 0) {
        p->velX = -p->velX;
        p->x = 0;
    }
    if (p->x >= g_window->bmp->width) {
        p->velX = -p->velX;
        p->x = g_window->bmp->width - 1;
    }

    // Update Y component of point.
    p->y += p->velY;
    if (p->y < 0) {
        p->velY = -p->velY;
        p->y = 0;
    }
    if (p->y >= g_window->bmp->height) {
        p->velY = -p->velY;
        p->y = g_window->bmp->height - 1;
    }
}


void BeziersMain() {
    g_window = CreateWin(1100, 900, WT_WINDOWED_FIXED, "Beziers Example");
  
    int const NUM_POINTS = 8;
    BezierPoint points[NUM_POINTS];
    for (int i = 0; i < NUM_POINTS; i++)
        BezierPointInit(&points[i]);

    DfColour ctrlLineColour = Colour(110, 110, 110);
    DfColour lineColour = Colour(230, 30, 50);

    while (!g_window->windowClosed && !g_window->input.keys[KEY_ESC]) {
        BitmapClear(g_window->bmp, g_colourBlack);
        InputPoll(g_window);

        for (int i = 0; i < NUM_POINTS; i++)
            BezierPointAdvance(&points[i]);

        for (int i = 0; i < NUM_POINTS; i += 2) {
            int j = (i + 1) % NUM_POINTS;
            int k = (i + 2) % NUM_POINTS;
            int l = (i + 3) % NUM_POINTS;
            double x1 = (points[i].x + points[j].x) * 0.5;
            double y1 = (points[i].y + points[j].y) * 0.5;
            double x2 = (points[k].x + points[l].x) * 0.5;
            double y2 = (points[k].y + points[l].y) * 0.5;
            int a[2] = { x1, y1 };
            int b[2] = { points[j].x, points[j].y };
            int c[2] = { points[k].x, points[k].y };
            int d[2] = { x2, y2 };

            if (g_window->input.keys[KEY_SPACE]) {
                DrawLine(g_window->bmp, a[0], a[1], b[0], b[1], ctrlLineColour);
                DrawLine(g_window->bmp, c[0], c[1], d[0], d[1], ctrlLineColour);
                CircleOutline(g_window->bmp, a[0], a[1], 2, g_colourWhite);
                CircleOutline(g_window->bmp, b[0], b[1], 2, g_colourWhite);
                CircleOutline(g_window->bmp, c[0], c[1], 2, g_colourWhite);
                CircleOutline(g_window->bmp, d[0], d[1], 2, g_colourWhite);
            }

            DrawBezier(g_window->bmp, a, b, c, d, lineColour);
        }

        UpdateWin(g_window);
        WaitVsync();
    }
}

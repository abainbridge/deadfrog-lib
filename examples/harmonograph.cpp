#include "df_bitmap.h"
#include "df_bmp.h"
#include "df_time.h"
#include "df_font.h"
#include "df_window.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


struct Point
{
    double x, y;
};


struct Pendulum
{
    double len;
    double theta;       // Angle of swing - 0=vertically down
    double vel;
    double fric;        // Amount of friction
    double xDisp;       // Displacement from the vertical
    double yDisp;
};


#define BIG_BITMAP_MULTIPLE 3

double g_advanceTime;
Pendulum g_pendula[6];
Point g_stickIntersectionPos = { 0, 0 };
DfBitmap *g_bigBmp;


// Find the points where the two circles intersect.
bool FindCircleCircleIntersections(
    Point p, float radius0,
    Point o, float radius1,
    Point *intersection1, Point *intersection2)
{
    // Find the distance between the centers.
    float dx = p.x - o.x;
    float dy = p.y - o.y;
    double dist = sqrtf(dx * dx + dy * dy);

    // See how many solutions there are.
    if (dist > radius0 + radius1)
    {
        return false;
    }
    else if (dist < fabs(radius0 - radius1))
    {
        // No solutions, one circle contains the other.
        return false;
    }
    else if (dist == 0 && radius0 == radius1)
    {
        // No solutions, the circles coincide.
        return false;
    }
    else
    {
        // Find a and h.
        double a = (radius0 * radius0 - radius1 * radius1 + dist * dist) / (2 * dist);
        double h = sqrtf(radius0 * radius0 - a * a);

        // Find P2.
        double cx2 = p.x + a * (o.x - p.x) / dist;
        double cy2 = p.y + a * (o.y - p.y) / dist;

        // Get the points P3.
        intersection1->x = cx2 + h * (o.y - p.y) / dist;
        intersection1->y = cy2 - h * (o.x - p.x) / dist;
        intersection2->x = cx2 - h * (o.y - p.y) / dist;
        intersection2->y = cy2 + h * (o.x - p.x) / dist;

        return true;
    }
}


void AdvancePendula()
{
    for (int i = 0; i < 6; i++)
    {
        Pendulum *p = g_pendula + i;
        double grav = 160.0;
        double accl = -grav / p->len * sin(p->theta);
        p->theta += (p->vel + g_advanceTime * accl / 2) * g_advanceTime;
        p->vel += accl * g_advanceTime;
        p->vel *= 1.0 - p->fric * g_advanceTime;
        p->xDisp = p->len * sin(p->theta);
        p->yDisp = p->len * cos(p->theta);
    }
}


void DrawCircle(DfBitmap *bmp, int x, int y, int r)
{
    float const twopi = 3.14159265f * 2.0f;
    for (float t = 0; t < twopi; t += 0.003f)
        PutPix(bmp, x + sin(t) * r, y + cos(t) * r, g_colourWhite);
}


double GetDistance(Point a, Point b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}


double drand(double range)
{
    return range * rand() / (double)RAND_MAX;
}


void InitPendula()
{
    for (int i = 0; i < 6; i++)
    {
        Pendulum *p = g_pendula + i;
        p->len = 200.0;
        p->theta = drand(1.0) - 0.5;
        p->vel = drand(2.0) - 1.0;
        p->fric = drand(0.015) + 0.0006;
    }

    g_stickIntersectionPos.x = 400.0;
    g_stickIntersectionPos.y = 0.0;
}


void RunSim()
{
    double const zoom = g_window->bmp->width * 0.6e-3 * (double)BIG_BITMAP_MULTIPLE;
    double xOffset = 140.0;
    double yOffset = 290.0;

    bool pendulaAtRest = true;
    for (int i = 0; i < 6; i++)
    {
        if (fabs(g_pendula[i].theta) > 0.001 || fabs(g_pendula[i].vel) > 0.001)
            pendulaAtRest = false;
    }
    if (pendulaAtRest)
        return;

    int y = g_window->bmp->height - 220;
    //        RectFill(g_window->backBuffer, 0, y, g_window->width, g_window->height - y, g_colourBlack);
    //        BitmapClear(g_window->backBuffer, g_colourBlack);

    for (int i = 0; i < 4000; i++)
    {
        AdvancePendula();

        // Project pendulums onto table's plane
        Point p1 = { 300 + g_pendula[0].xDisp, 300 + g_pendula[1].xDisp };
        Point p2 = { 600 + g_pendula[2].xDisp, 600 + g_pendula[3].xDisp };
        //             PutPix(g_window->backBuffer, p1.x, p1.y, g_colourWhite);
        //             PutPix(g_window->backBuffer, 300, 300, g_colourWhite);
        //             PutPix(g_window->backBuffer, p2.x, p2.y, g_colourWhite);
        //             PutPix(g_window->backBuffer, 600, 600, g_colourWhite);

        // Draw Circles 
        double stickLen = 400;
        //             DrawCircle(g_window->backBuffer, p1.x, p1.y, stickLen);
        //             DrawCircle(g_window->backBuffer, p2.x, p2.y, stickLen);

        Point i1, i2;
        if (FindCircleCircleIntersections(p1, stickLen, p2, stickLen, &i1, &i2))
        {
            double i1dist = GetDistance(i1, g_stickIntersectionPos);
            double i2dist = GetDistance(i2, g_stickIntersectionPos);
            if (i2dist < i1dist)
                i1 = i2;
            g_stickIntersectionPos = i1;

            // Apply the effect of the pendulums that move the paper
            i1.x -= g_pendula[4].xDisp * 2.0;
            i1.y -= g_pendula[5].xDisp * 2.0;

            // Enlarge and centre the result on the bitmap
            i1.x = (i1.x + xOffset) * zoom;
            i1.y = (i1.y + yOffset) * zoom;

            DfColour c = GetPix(g_bigBmp, i1.x, i1.y);
            c.b *= 48/64.0;
            c.g *= 63/64.0;
            c.r *= 60/64.0;
            c.b += 64;
            c.g += 4;
            c.r += 16;

            PutPix(g_bigBmp, i1.x, i1.y, c);
            PutPix(g_window->bmp, i1.x/(double)BIG_BITMAP_MULTIPLE, i1.y/(double)BIG_BITMAP_MULTIPLE, c);
        }
    }
}


void HarmonographMain()
{
    // Setup the window
    int width = 1024;
    int height = width * 9 / 16;
    GetDesktopRes(&width, &height);
    CreateWin(width, height, WT_WINDOWED, "Harmonograph");

    g_bigBmp = BitmapCreate(width * BIG_BITMAP_MULTIPLE, height * BIG_BITMAP_MULTIPLE);
    DfFont *font = FontCreate("Lucida Console", 10, 4);

    BitmapClear(g_window->bmp, g_colourBlack);
    BitmapClear(g_bigBmp, g_colourBlack);

    g_advanceTime = 0.004 / (double)BIG_BITMAP_MULTIPLE;

    InitPendula();

    // Continue to display the window until the user presses escape or clicks the close icon
    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        InputManagerAdvance();
        
        if (g_input.keyDowns[KEY_SPACE])
        {
            InitPendula();
            BitmapClear(g_window->bmp, g_colourBlack);
            BitmapClear(g_bigBmp, g_colourBlack);
        }
        if (g_input.keyDowns[KEY_S])
        {
            SaveBmp(g_bigBmp, "foo.bmp");
        }

        RunSim();

        DrawTextSimple(font, g_colourWhite, g_window->bmp, 5, height - 20, "Keys: Space to restart  S to save");

        UpdateWin();
        SleepMillisec(1);
    }
}

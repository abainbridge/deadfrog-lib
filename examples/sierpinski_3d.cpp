#include "df_font.h"
#include "df_window.h"

#include <math.h>
#include <stdlib.h>


struct Point
{
    float x, y, z;
};


#define NUM_POINTS 1000000

Point g_points[NUM_POINTS];
Point g_cameraPos = { 0.0f, 0.0f, -100.0f };
float g_rotationX = 0.0f;


static void CreateSierpinski3D()
{
	float size = 20.0f;
    Point corners[5] = {
        { 0, 0, size },
        { size,  size, -size },
        { size, -size, -size },
        { -size,  size, -size },
        { -size, -size, -size }
    };

    Point p = corners[0];
 	
    for (unsigned i = 0; i < NUM_POINTS; i++)
    {
        Point p2 = corners[rand() % 5];
        p.x = (p.x + p2.x) / 2;
        p.y = (p.y + p2.y) / 2;
        p.z = (p.z + p2.z) / 2;

        g_points[i] = p;
    }
}


static void Render()
{
    float scale = 1.0f * g_window->bmp->width;

    float halfWidth = g_window->bmp->width / 2;
    float halfHeight = g_window->bmp->height / 2;

    float cosRotX = cos(g_rotationX);
    float sinRotX = sin(g_rotationX);

    for (int i = 0; i < NUM_POINTS; i++)
    {
        Point p = g_points[i];
        
        // Spin in model-view space
        float tmp = p.x;
        p.x = cosRotX * p.x + sinRotX * p.y;
        p.y = -sinRotX * tmp + cosRotX * p.y;

        // Move into view space
        p.y -= g_cameraPos.z;

        tmp = scale / p.y;

        unsigned x = p.x * tmp + halfWidth;
        unsigned y = -p.z * tmp + halfHeight;

        if (x < g_window->bmp->width && y < g_window->bmp->height)
        {
            unsigned brightness = 15;
            unsigned char invA = 255 - brightness;
            DfColour *pixel = g_window->bmp->lines[y] + x;
            pixel->r = (pixel->r * invA + 255 * brightness) >> 8;
            pixel->g = pixel->r;
            pixel->b = pixel->r;
        }
    }
}


void Sierpinski3DMain()
{
    CreateWin(800, 600, WT_WINDOWED, "Sierpinski Gasket Example");
    DfFont *font = DfCreateFont("Courier New", 12, 4);

    CreateSierpinski3D();

    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        ClearBitmap(g_window->bmp, g_colourBlack);
        InputManagerAdvance();

        if (g_input.keys[KEY_UP])
            g_cameraPos.z += 20.0f * g_window->advanceTime;
        if (g_input.keys[KEY_DOWN])
            g_cameraPos.z -= 20.0f * g_window->advanceTime;
        if (g_input.keys[KEY_LEFT])
            g_rotationX += 0.9f * g_window->advanceTime;
        if (g_input.keys[KEY_RIGHT])
            g_rotationX -= 0.9f * g_window->advanceTime;

        Render();

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        UpdateWin();
//        DfSleepMillisec(10);
    }
}

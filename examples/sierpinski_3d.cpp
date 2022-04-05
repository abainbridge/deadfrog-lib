#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

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


static void Render(DfWindow *win)
{
    float scale = 1.0f * win->bmp->width;

    float halfWidth = win->bmp->width / 2;
    float halfHeight = win->bmp->height / 2;

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

        if (x < win->bmp->width && y < win->bmp->height)
        {
            unsigned brightness = 15;
            unsigned char invA = 255 - brightness;
            DfColour *pixel = &win->bmp->pixels[y * win->bmp->width] + x;
            pixel->r = (pixel->r * invA + 255 * brightness) >> 8;
            pixel->g = pixel->r;
            pixel->b = pixel->r;
        }
    }
}


void Sierpinski3DMain()
{
    DfWindow *win = CreateWin(800, 600, WT_WINDOWED_RESIZEABLE, "Sierpinski Gasket Example");
    BitmapClear(win->bmp, g_colourBlack);
    DfFont *font = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    CreateSierpinski3D();

    while (!win->windowClosed && !win->input.keyDowns[KEY_ESC])
    {
        BitmapClear(win->bmp, g_colourBlack);
        InputPoll(win);

        if (win->input.keys[KEY_UP])
            g_cameraPos.z += 20.0f * win->advanceTime;
        if (win->input.keys[KEY_DOWN])
            g_cameraPos.z -= 20.0f * win->advanceTime;
        if (win->input.keys[KEY_LEFT])
            g_rotationX += 0.9f * win->advanceTime;
        if (win->input.keys[KEY_RIGHT])
            g_rotationX -= 0.9f * win->advanceTime;

        Render(win);

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, win->bmp, win->bmp->width - 5, 0, "FPS:%i", win->fps);

        UpdateWin(win);
//        WaitVsync();
    }
}

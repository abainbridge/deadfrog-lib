#include <math.h>
#include <windows.h>

#include "lib/gfx/bitmap.h"
#include "lib/gfx/graph3d.h"
#include "lib/gfx/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/window_manager.h"


static float frand(float _max)
{
    return _max * (float)rand() / (float)RAND_MAX;
}


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE, LPSTR cmdLine, int)
{
	WindowData *win = CreateWin(100, 100, 800, 800, true, "test");
    TextRenderer *font = CreateTextRenderer("Courier", 8);
    Graph3d *graph3d = CreateGraph3d();

    for (int i = 0; i < 170000; i++)
        Graph3dAddPoint(graph3d, frand(100.0f) - 50.0f, 
                                 frand(100.0f) - 50.0f, 
                                 frand(100.0f) - 50.0f, rand() * rand());

    int screenW = GetWidth(win);
    int screenH = GetHeight(win);
    BitmapRGBA *bmp = win->bmp;

    float rotX = 0.0f;
    float rotZ = 0.0f;
    while (!win->inputManager->keyDowns[KEY_ESC] && !win->windowClosed)
    {
        // Read user input etc
        AdvanceWin(win);

        ClearBitmap(bmp, g_colourBlack);

        DrawTextLeft(font, g_colourWhite, bmp, screenW - 100, 5, "FPS: %d", win->fps);

        if (win->inputManager->lmb)
        {
            rotX -= float(win->inputManager->mouseVelY) * 0.01f;
            rotZ += float(win->inputManager->mouseVelX) * 0.01f;
        }

        Graph3dRender(graph3d, bmp, 400, 400, 200, 800, g_colourWhite, rotX, rotZ);

        DrawTextLeft(font, g_colourWhite, bmp, 0, screenH - 15, "Hold left mouse to rotate");
    }
}

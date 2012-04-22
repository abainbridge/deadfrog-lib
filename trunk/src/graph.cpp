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
    TextRenderer *font = CreateTextRenderer("SmallBold6", 8);
    Graph3d *graph3d = CreateGraph3d();

    for (int i = 0; i < 170000; i++)
        Graph3dAddPoint(graph3d, frand(100.0f) - 50.0f, 
                                 frand(100.0f) - 50.0f, 
                                 frand(100.0f) - 50.0f, rand() * rand());

    int screenW = GetWidth(win);
    int screenH = GetHeight(win);
    BitmapRGBA *bmp = win->bmp;

    float rotZ = 0.0f;
    while (!win->inputManager->g_keyDowns[KEY_ESC] && !win->windowClosed)
    {
        // Read user input etc
        AdvanceWin(win);

        ClearBitmap(bmp, g_colourBlack);

        DrawTextLeft(font, g_colourWhite, bmp, screenW - 100, 5, "FPS: %d", win->fps);
//         DrawTextLeft(font, g_colourWhite, bmp, 5, 525, "abcdefghijklmnopqrstuvwxyz!@¬!\"£$%^&*()_+{}:", win->fps);
//         DrawTextLeft(font, g_colourWhite, bmp, 5, 545, "ABCDEFGHIJKLMNOPQRSTUVWXYZ`1234567890@~<>?`-=[];'#,./\\|", win->fps);

// 		for (int i = 0; i < 1000 * 1000 * 10; i++)
// 		{
// //			DfPutPixel(bmp, rand() & 511, rand() & 511, g_colourWhite);
// 			DfPutPixel(bmp, i & 511, i & 511, g_colourWhite);
// 		}

        rotZ += 0.03f;
        Graph3dRender(graph3d, bmp, 400, 400, 200, 800, g_colourWhite, rotZ);

//         for (int i = 0; i < 30; i++)
//             DrawTextSimple(font, g_colourWhite, bmp, rand() % screenW, (rand() & 511) + 100, "Hello World!");

//         int mx = win->inputManager->m_mouseX;
//         int my = win->inputManager->m_mouseY;
//         DrawTextCentre(font, 0xffff00ff, bmp, mx, my - 10, "Mouse");
    }
}

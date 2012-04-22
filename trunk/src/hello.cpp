#include <windows.h>

#include "lib/gfx/text_renderer.h"
#include "lib/window_manager.h"


int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE, LPSTR cmdLine, int)
{
	WindowData *win = CreateWin(100, 100, 800, 600, true, "Hello World!");
    TextRenderer *font = CreateTextRenderer("Courier", 18);

    while (!win->windowClosed)
    {
        AdvanceWin(win); // Read user input, blit back buffer to screen, etc
        ClearBitmap(win->bmp, g_colourBlack);
        DrawTextCentre(font, g_colourWhite, win->bmp,
                       win->width/2, win->height/2,
                       "Hello World!");
    }
}

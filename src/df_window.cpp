#include "df_window.h"

#include "df_time.h"
#include "df_input.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shellapi.h>
#include <ShellScalingAPI.h>
#include <stdlib.h>


// Prototype of DwmFlush function from Win API.
typedef void (WINAPI DwmFlushFunc)();


struct WindowDataPrivate
{
    HWND				m_hWnd;
    unsigned int	    m_framesThisSecond;
    double			    m_endOfSecond;
    double              m_lastUpdateTime;
    DwmFlushFunc        *m_dwmFlush;
};


DfWindow *g_window = NULL;


// ****************************************************************************
// Window Proc
// ****************************************************************************

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DfWindow *win = g_window;

    if (message == WM_SYSKEYDOWN && wParam == 115)
        SendMessage(hWnd, WM_CLOSE, 0, 0);

    switch (message)
    {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        SetCapture(hWnd);
        break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        ReleaseCapture();
        break;
    }

    switch (message)
	{
        case WM_SIZE:
        {
            int w = lParam & 0xffff;
            int h = (lParam & 0xffff0000) >> 16;
            if (w != win->bmp->width || h != win->bmp->height)
            {
                BitmapDelete(win->bmp);
                win->bmp = BitmapCreate(w, h);
            }
            break;
        }

        case WM_PAINT:
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;

        case WM_CLOSE:
            if (win)
			    win->windowClosed = true;
			return 0;

		default:
			if (!win || EventHandler(message, wParam, lParam) == -1)
				return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


bool GetDesktopRes(int *width, int *height)
{
    HWND desktopWindow = GetDesktopWindow();
    RECT desktopRect;

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
    if (GetWindowRect(desktopWindow, &desktopRect) == 0)
        return false;
    if (width)
        *width = desktopRect.right - desktopRect.left;
    if (height)
        *height = desktopRect.bottom - desktopRect.top;
    return true;
}


bool CreateWin(int width, int height, WindowType winType, char const *winName)
{
	if (g_window)
        return false;

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    DfWindow *wd = g_window = new DfWindow;
	memset(wd, 0, sizeof(DfWindow));

    wd->_private = new WindowDataPrivate;
    memset(wd->_private, 0, sizeof(WindowDataPrivate));

	width = ClampInt(width, 100, 4000);
	height = ClampInt(height, 100, 4000);

	// Register window class
    WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = GetModuleHandle(0);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = winName;
	RegisterClass(&wc);

    wd->bmp = BitmapCreate(width, height);

    unsigned windowStyle = WS_VISIBLE;
	if (winType == WT_WINDOWED)
	{
		windowStyle |= WS_OVERLAPPEDWINDOW;

		RECT windowRect = { 0, 0, width, height };
		AdjustWindowRect(&windowRect, windowStyle, false);
		int borderWidth = (windowRect.right - windowRect.left) - width;
		int titleHeight = ((windowRect.bottom - windowRect.top) - height) - borderWidth;
		width += borderWidth;
		height += borderWidth + titleHeight;
	}
	else
	{
        windowStyle |= WS_POPUP;

		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmBitsPerPel = 32;
		devmode.dmPelsWidth = width;
		devmode.dmPelsHeight = height;
		devmode.dmDisplayFrequency = 60;
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		ReleaseAssert(ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL,
            "Couldn't change screen resolution to %i x %i", width, height);
	}

	// Create main window
	wd->_private->m_hWnd = CreateWindow(wc.lpszClassName, wc.lpszClassName,
		windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, 0/*g_hInstance*/, NULL);

    double now = GetRealTime();
    wd->_private->m_lastUpdateTime = now;
    wd->_private->m_endOfSecond = now + 1.0;

    HMODULE dwm = LoadLibrary("dwmapi.dll");
    wd->_private->m_dwmFlush = (DwmFlushFunc *)GetProcAddress(dwm, "DwmFlush");

    CreateInputManager();
	return true;
}


// This function copies a DfBitmap to the window, so you can actually see it.
// SetBIBitsToDevice seems to be the fastest way to achieve this on most hardware.
static void BlitBitmapToWindow(DfWindow *wd, DfBitmap *bmp)
{
    BITMAPINFO binfo;
    binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    binfo.bmiHeader.biWidth = bmp->width;
    binfo.bmiHeader.biHeight = -(int)bmp->height;
    binfo.bmiHeader.biPlanes = 1;
    binfo.bmiHeader.biBitCount = 32;
    binfo.bmiHeader.biCompression = BI_RGB;
    binfo.bmiHeader.biSizeImage = bmp->height * bmp->width * 4;
    binfo.bmiHeader.biXPelsPerMeter = 0;
    binfo.bmiHeader.biYPelsPerMeter = 0;
    binfo.bmiHeader.biClrUsed = 0;
    binfo.bmiHeader.biClrImportant = 0;

    HDC dc = GetDC(wd->_private->m_hWnd);

    SetDIBitsToDevice(dc,
        0, 0, bmp->width, bmp->height,
        0, 0, 0, bmp->height,
        bmp->pixels, &binfo, DIB_RGB_COLORS
    );

    ReleaseDC(wd->_private->m_hWnd, dc);
}


void UpdateWin()
{
    // *** FPS Meter ***

    g_window->_private->m_framesThisSecond++;

    double currentTime = GetRealTime();
    if (currentTime > g_window->_private->m_endOfSecond)
    {
        // If program has been paused by a debugger, skip the seconds we missed
        if (currentTime > g_window->_private->m_endOfSecond + 2.0)
            g_window->_private->m_endOfSecond = currentTime + 1.0;
        else
            g_window->_private->m_endOfSecond += 1.0;
        g_window->fps = g_window->_private->m_framesThisSecond;
        g_window->_private->m_framesThisSecond = 0;
    }
    else if (g_window->_private->m_endOfSecond > currentTime + 2.0)
    {
        g_window->_private->m_endOfSecond = currentTime + 1.0;
    }


    // *** Adance time ***

    g_window->advanceTime = currentTime - g_window->_private->m_lastUpdateTime;
    g_window->_private->m_lastUpdateTime = currentTime;


    // *** Swap buffers ***

    BlitBitmapToWindow(g_window, g_window->bmp);
}


void ShowMouse()
{
	ShowCursor(true);
}


void HideMouse()
{
	ShowCursor(false);
}


bool WaitVsync()
{
    if (g_window->_private->m_dwmFlush) {
        g_window->_private->m_dwmFlush();
        return true;
    }

    return false;
}

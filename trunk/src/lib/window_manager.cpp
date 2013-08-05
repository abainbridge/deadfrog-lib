#include "window_manager.h"

#include "lib/hi_res_time.h"
#include "lib/input.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shellapi.h>
#include <stdlib.h>


struct WindowDataPrivate
{
    HWND				m_hWnd;
    int					m_borderWidth;
    int					m_titleHeight;
    HINSTANCE			m_hInstance;
    unsigned int	    m_framesThisSecond;
    double			    m_endOfSecond;
};


WindowData *g_window = NULL;


// ****************************************************************************
// Window Proc
// ****************************************************************************

#define FULLSCREEN_WINDOW_STYLE (WS_POPUPWINDOW | WS_VISIBLE)
#define WINDOWED_WINDOW_STYLE (FULLSCREEN_WINDOW_STYLE | (WS_CAPTION | WS_BORDER))
//#define WINDOWED_WINDOW_STYLE (FULLSCREEN_WINDOW_STYLE | (WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX))

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowData *win = g_window;

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


static int clamp(int val, int min, int max)
{
	if (val < min)
		return min;
	if (val > max)
		return max;
	return val;
}


bool CreateWin(int x, int y, int width, int height, bool _windowed, char const *winName)
{
	if (g_window)
        return false;
    WindowData *wd = g_window = new WindowData;
	memset(wd, 0, sizeof(WindowData));
    
    wd->_private = new WindowDataPrivate;
    memset(wd->_private, 0, sizeof(WindowDataPrivate));

    static int const minWidth = 160;
    static int const minHeight = 120;
	width = clamp(width, minWidth, 3000);
	height = clamp(height, minHeight, 2300);
	wd->width = width;
	wd->height = height;

	wd->_private->m_hInstance = GetModuleHandle(0);

	// Register window class
	WNDCLASS wc;
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = wd->_private->m_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = winName;
	RegisterClass(&wc);

	unsigned int windowStyle = FULLSCREEN_WINDOW_STYLE;
	if (_windowed)
	{
		windowStyle = WINDOWED_WINDOW_STYLE;

		RECT windowRect = { 0, 0, width, height };
		AdjustWindowRect(&windowRect, windowStyle, false);
		wd->_private->m_borderWidth = ((windowRect.right - windowRect.left) - width) / 2;
		wd->_private->m_titleHeight = ((windowRect.bottom - windowRect.top) - height) - wd->_private->m_borderWidth * 2;
		width += wd->_private->m_borderWidth * 2;
		height += wd->_private->m_borderWidth * 2 + wd->_private->m_titleHeight;

		HWND desktopWindow = GetDesktopWindow();
		RECT desktopRect;
		GetWindowRect(desktopWindow, &desktopRect);
		
        // Make sure a reasonable portion of the window is visible
        int minX = 100 - wd->width;
        int maxX = desktopRect.right - 100;
        x = clamp(x, minX, maxX);
		y = clamp(y, 0, desktopRect.bottom - minHeight);
	}
	else
	{
		DEVMODE devmode;
		devmode.dmSize = sizeof(DEVMODE);
		devmode.dmBitsPerPel = 32;
		devmode.dmPelsWidth = width;
		devmode.dmPelsHeight = height;
		devmode.dmDisplayFrequency = 60;
		devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		ReleaseAssert(ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL,
            "Couldn't change screen resolution to %i x %i", width, height);
        
		x = 0;
		y = 0;
		wd->_private->m_borderWidth = 1;
		wd->_private->m_titleHeight = 0;
	}

    wd->backBuffer = CreateBitmapRGBA(width, height);

	// Create main window
	wd->_private->m_hWnd = CreateWindow(wc.lpszClassName, wc.lpszClassName, 
		windowStyle, x, y, width, height,
		NULL, NULL, 0/*g_hInstance*/, NULL);

    wd->_private->m_endOfSecond = GetHighResTime() + 1.0;

    CreateInputManager();
	return true;
}


// This function copies a BitmapRGBA to the window, so you can actually see it.
// SetBIBitsToDevice seems to be the fastest way to achieve this on most hardware.
static void BlitBitmapToWindow(WindowData *wd, BitmapRGBA *bmp, int x, int y)
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
        x, y, bmp->width, bmp->height,
        0, 0, 0, bmp->height,
        bmp->pixels, &binfo, DIB_RGB_COLORS
    );

    ReleaseDC(wd->_private->m_hWnd, dc);
}


void AdvanceWin()
{
    // *** Input Manager ***
    
    InputManagerAdvance();

    
    // *** FPS Meter ***
    
    g_window->_private->m_framesThisSecond++;

    double currentTime = GetHighResTime();
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

    BlitBitmapToWindow(g_window, g_window->backBuffer, 0, 0);
}


void ShowMouse()
{
	ShowCursor(true);
}


void HideMouse()
{
	ShowCursor(false);
}
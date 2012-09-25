#include "window_manager.h"

#include <limits.h>
#include <windows.h>
#include <shellapi.h>
#include <stdlib.h>	// For exit()

#include "lib/gfx/bitmap.h"
#include "lib/gfx/text_renderer.h"
#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/message_dialog.h"


struct WindowDataPrivate
{
    HWND				m_hWnd;
    int					m_borderWidth;
    int					m_titleHeight;
    HINSTANCE			m_hInstance;

    unsigned int	    m_framesThisSecond;
    double			    m_endOfSecond;
};



// ****************************************************************************
//  Window manager
// ****************************************************************************

#define MAX_WINDOWS 100
WindowData *g_windows[MAX_WINDOWS] = { NULL };


WindowData *GetWindowData(HWND hwnd)
{
    for (int i = 0; i < MAX_WINDOWS; i++)
    {
        if (g_windows[i] && g_windows[i]->_private->m_hWnd == hwnd)
            return g_windows[i];
    }

    return NULL;
}


void AddWindowData(WindowData *w)
{
    for (int i = 0; i < MAX_WINDOWS; i++)
    {
        if (!g_windows[i])
        {
            g_windows[i] = w;
            break;
        }
    }
}


void RemoveWindowData(WindowData *w)
{
    for (int i = 0; i < MAX_WINDOWS; i++)
    {
        if (g_windows[i] == w)
        {
            g_windows[i] = NULL;
            break;
        }
    }
}


// ****************************************************************************
// Window Proc
// ****************************************************************************

#define FULLSCREEN_WINDOW_STYLE (WS_POPUPWINDOW | WS_VISIBLE)
#define WINDOWED_WINDOW_STYLE (FULLSCREEN_WINDOW_STYLE | (WS_CAPTION | WS_BORDER))
//#define WINDOWED_WINDOW_STYLE (FULLSCREEN_WINDOW_STYLE | (WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX))

#include "debug_utils.h"
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowData *win = GetWindowData(hWnd);

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

// 		case WM_NCCALCSIZE:
// 		{
// 			if (wParam == 0)
// 			{
// 				// This branch is taken on startup
// 				RECT *rect = (RECT *)lParam;
// 				*s_width = rect->right - rect->left;
// 				*s_height = rect->bottom - rect->top;
// 			}
// 			else
// 			{
// 				// This branch is taken otherwise
// 				NCCALCSIZE_PARAMS *csp = (NCCALCSIZE_PARAMS*)lParam;
// 				*s_width = csp->lppos->cx;
// 				*s_height = csp->lppos->cy;
// 			}
// 			*s_width -= 2 * g_windowManager->GetBorderWidth();
// 			*s_height -= 2 * g_windowManager->GetBorderWidth();
// 			*s_height -= g_windowManager->GetTitleHeight();
//             if ((int)(*s_height) < 0)
//                 *s_height = 0;
// 			return DefWindowProc( hWnd, message, wParam, lParam );
// 			break;
// 		}

        case WM_CLOSE:
            if (win)
			    win->windowClosed = true;
			return 0;

		default:
			if (!win || EventHandler(win->inputManager, message, wParam, lParam) == -1)
			{
				return DefWindowProc(hWnd, message, wParam, lParam);
			}
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


WindowData *CreateWin(int x, int y, int width, int height, bool _windowed, char const *winName)
{
	WindowData *wd = new WindowData;
	memset(wd, 0, sizeof(WindowData));
    
    wd->_private = new WindowDataPrivate;
    memset(wd->_private, 0, sizeof(WindowDataPrivate));

    wd->inputManager = CreateInputManager();

    static int const minWidth = 320;
    static int const minHeight = 240;
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
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = winName;
	RegisterClass( &wc );

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
		ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);
		x = 0;
		y = 0;
		wd->_private->m_borderWidth = 1;
		wd->_private->m_titleHeight = 0;
	}

    wd->bmp = CreateBitmapRGBA(width, height);

	// Create main window
	wd->_private->m_hWnd = CreateWindow( 
		wc.lpszClassName, wc.lpszClassName, 
		windowStyle,
		x, y, width, height,
		NULL, NULL, 0/*g_hInstance*/, NULL );

// 	// Set the window's title bar icon
// 	HANDLE hIcon = LoadImageA(g_hInstance, "#101", IMAGE_ICON, 16, 16, 0);
//     if (hIcon)
//     {
//         SendMessage(m_hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
//         SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
//     }
// 	hIcon = LoadImageA(g_hInstance, "#101", IMAGE_ICON, 32, 32, 0);
//     if (hIcon)
//     {
//         SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
// 	}

    AddWindowData(wd);

    wd->_private->m_endOfSecond = GetHighResTime() + 1.0;

	return wd;
}


BitmapRGBA *AdvanceWin(WindowData *w)
{
    // *** Input Manager ***
    
    InputManagerAdvance(w->inputManager);

    
    // *** FPS Meter ***
    
    w->_private->m_framesThisSecond++;

    double currentTime = GetHighResTime();
    if (currentTime > w->_private->m_endOfSecond)
    {
        if (currentTime > w->_private->m_endOfSecond + 2.0)
        {
            w->_private->m_endOfSecond = currentTime + 1.0;
        }
        else
        {
            w->_private->m_endOfSecond += 1.0;
        }
        w->fps = w->_private->m_framesThisSecond;
        w->_private->m_framesThisSecond = 0;
    }
    else if (w->_private->m_endOfSecond > currentTime + 2.0)
    {
        w->_private->m_endOfSecond = currentTime + 1.0;
    }

    BlitBitmap(w, w->bmp, 0, 0);
    SleepEx(10, true);

    return w->bmp;
}


unsigned int GetWidth(WindowData *w)
{
	return w->width;
}


unsigned int GetHeight(WindowData *w)
{
	return w->height;
}


void BlitBitmap(WindowData *wd, BitmapRGBA *bmp, int x, int y)
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
		x, y,
		bmp->width, bmp->height,
		0, 0,
		0, bmp->height,
		bmp->pixels,
		&binfo,
		DIB_RGB_COLORS
		);

	ReleaseDC(wd->_private->m_hWnd, dc);
}


void ShowMouse()
{
	ShowCursor(true);
}


void HideMouse()
{
	ShowCursor(false);
}


void SetWindowTitle(WindowData *wd, char const *title)
{
	SetWindowText(wd->_private->m_hWnd, title);
}

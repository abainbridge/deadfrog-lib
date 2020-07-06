// System headers.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <ShellScalingAPI.h>

// Standard headers.
#include <memory.h>


// Prototype of DwmFlush function from Win API.
typedef void (WINAPI DwmFlushFunc)();


static HWND g_hWnd = 0;
static DwmFlushFunc *g_dwmFlush = NULL;



// Returns 0 if the event is handled here, -1 otherwise
int EventHandler(unsigned int message, unsigned int wParam, int lParam)
{
    static char const *s_keypressOutOfRangeMsg = "Keypress value out of range (%s: wParam = %d)";

	switch (message)
	{
        case WM_ACTIVATE:
            break;
		case WM_SETFOCUS:
		case WM_NCACTIVATE:
			g_input.windowHasFocus = true;
			// Clear keyboard state when we regain focus
            memset(g_priv.m_keyNewDowns, 0, KEY_MAX);
            memset(g_priv.m_keyNewUps, 0, KEY_MAX);
            memset(g_input.keys, 0, KEY_MAX);
            return -1;
		case WM_KILLFOCUS:
			g_input.windowHasFocus = false;
            g_input.mouseX = -1;
            g_input.mouseY = -1;
			break;

        case WM_SYSCHAR:
            break;

		case WM_CHAR:
			if (g_input.numKeysTyped < MAX_KEYS_TYPED_PER_FRAME &&
				!g_input.keys[KEY_CONTROL] &&
				!g_input.keys[KEY_ALT] &&
				wParam != KEY_ESC)
			{
                ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_CHAR", wParam);
				g_priv.m_newKeysTyped[g_priv.m_newNumKeysTyped] = wParam;
				g_priv.m_newNumKeysTyped++;
			}
			break;

		case WM_LBUTTONDOWN:
			g_priv.m_lmbPrivate = true;
			break;
		case WM_LBUTTONUP:
			g_priv.m_lmbPrivate = false;
			break;
		case WM_MBUTTONDOWN:
			g_input.mmb = true;
			break;
		case WM_MBUTTONUP:
			g_input.mmb = false;
			break;
		case WM_RBUTTONDOWN:
			g_input.rmb = true;
			break;
		case WM_RBUTTONUP:
			g_input.rmb = false;
			break;

		/* Mouse clicks in the Non-client area (ie titlebar) of the window are weird. If we
		   handle them and return 0, then Windows ignores them and it is no longer possible
		   to drag the window by the title bar. If we handle them and return -1, then Windows
		   never sends us the button up event. I've chosen the second option and fix some of
		   the brokenness by generating a fake up event one frame after the down event. */
		case WM_NCLBUTTONDOWN:
			g_priv.m_lmbPrivate = true;
			g_priv.m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCMBUTTONDOWN:
			g_input.mmb = true;
			g_priv.m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCRBUTTONDOWN:
			g_input.rmb = true;
			g_priv.m_lastClickWasNC = true;
			return -1;
			break;

		case WM_MOUSEWHEEL:
		{
			int move = (short)HIWORD(wParam) / 120;
			g_input.mouseZ += move;
			break;
		}

		case WM_NCMOUSEMOVE:
			g_input.mouseX = -1;
			g_input.mouseY = -1;
			break;

		case WM_MOUSEMOVE:
		{
			short newPosX = lParam & 0xFFFF;
			short newPosY = short(lParam >> 16);
			g_input.mouseX = newPosX;
			g_input.mouseY = newPosY;
			break;
		}

		case WM_SYSKEYUP:
            ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_SYSKEYUP", wParam);

            // If the key event is the Control part of Alt_Gr, throw it away. There
            // will be an event for the alt part too.
            if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
                break;

            g_input.keys[wParam] = 0;
            g_priv.m_keyNewUps[wParam] = 1;
            break;

		case WM_SYSKEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_SYSKEYDOWN", wParam);
			//int flags = (short)HIWORD(lParam);
			g_input.keys[wParam] = 1;
			g_priv.m_keyNewDowns[wParam] = 1;
			break;
		}

		case WM_KEYUP:
		{
			// Alt key ups are presented here when the user keys, for example, Alt+F.
			// Windows will generate a SYSKEYUP event for the release of F, and a
			// normal KEYUP event for the release of the ALT. Very strange.
			ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_KEYUP", wParam);
            g_priv.m_keyNewUps[wParam] = 1;
            g_input.keys[wParam] = 0;
			break;
		}

		case WM_KEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_KEYDOWN", wParam);
			if (wParam == KEY_DEL)
			{
				g_priv.m_newKeysTyped[g_input.numKeysTyped] = 127;
				g_priv.m_newNumKeysTyped++;
			}
            else if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
            {
                //int res = GetAsyncKeyState(VK_MENU);
                // Key event is the Control part of Alt_Gr, so throw it away. There
                // will be an event for the alt part too.
                break;
            }
			g_priv.m_keyNewDowns[wParam] = 1;
			g_input.keys[wParam] = 1;
			break;
		}

		default:
			return -1;
	}

    g_input.eventSinceAdvance = true;
	return 0;
}


bool InputPoll()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
    {
        // handle or dispatch messages
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    } 

    InputPollInternal();

    bool rv = g_input.eventSinceAdvance;
    g_input.eventSinceAdvance = false;
    return rv;
}



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


#pragma comment(lib, "Shcore") // Needed for SetProcessDpiAwareness
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
	g_hWnd = CreateWindow(wc.lpszClassName, wc.lpszClassName,
		windowStyle, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
		NULL, NULL, 0/*g_hInstance*/, NULL);

    double now = GetRealTime();
    g_lastUpdateTime = now;
    g_endOfSecond = now + 1.0;

    HMODULE dwm = LoadLibrary("dwmapi.dll");
    g_dwmFlush = (DwmFlushFunc *)GetProcAddress(dwm, "DwmFlush");

    InitInput();
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

    HDC dc = GetDC(g_hWnd);

    SetDIBitsToDevice(dc,
        0, 0, bmp->width, bmp->height,
        0, 0, 0, bmp->height,
        bmp->pixels, &binfo, DIB_RGB_COLORS
    );

    ReleaseDC(g_hWnd, dc);
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
    if (g_dwmFlush) {
        g_dwmFlush();
        return true;
    }

    return false;
}

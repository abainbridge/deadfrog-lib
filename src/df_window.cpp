// Own header.
#include "df_window.h"

// Project headers.
#include "df_time.h"

// System headers.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <ShellScalingAPI.h>

// Standard headers.
#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>


struct InputPrivate
{
    bool        m_lmbPrivate;
    bool		m_lmbOld;				// Mouse button states from last frame. Only
    bool		m_mmbOld;				// used to computer the deltas.
    bool		m_rmbOld;

    bool		m_lastClickWasNC;		// True if mouse was outside the client area of the window when the button was last clicked
    double		m_lastClickTime;		// Used in double click detection
    int			m_lmbRepeatsSoFar;		// When lmb is held, this value is used by IsLmbRepeatSet to determine when to return true

    int			m_mouseOldX;			// Same things but for position
    int			m_mouseOldY;
    int			m_mouseOldZ;

    signed char	m_keyNewDowns[KEY_MAX];
    signed char	m_keyNewUps[KEY_MAX];
    char		m_newKeysTyped[MAX_KEYS_TYPED_PER_FRAME];
    int			m_newNumKeysTyped;
};


// Prototype of DwmFlush function from Win API.
typedef void (WINAPI DwmFlushFunc)();


DfInput g_input;
static InputPrivate g_priv = { 0 };

static HWND g_hWnd = 0;
static unsigned g_framesThisSecond = 0;
static double g_endOfSecond = 0.0;
static double g_lastUpdateTime = 0.0;
static DwmFlushFunc *g_dwmFlush = NULL;

DfWindow *g_window = NULL;



void CreateInputManager()
{
    memset(&g_input, 0, sizeof(DfInput));

    g_input.eventSinceAdvance = true;
    g_input.windowHasFocus = true;
}


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


bool InputManagerAdvance()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) 
    {
        // handle or dispatch messages
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    } 

    memcpy(g_input.keyDowns, g_priv.m_keyNewDowns, KEY_MAX);
    memset(g_priv.m_keyNewDowns, 0, KEY_MAX);
    memcpy(g_input.keyUps, g_priv.m_keyNewUps, KEY_MAX);
    memset(g_priv.m_keyNewUps, 0, KEY_MAX);

	g_input.numKeysTyped = g_priv.m_newNumKeysTyped;
	memcpy(g_input.keysTyped, g_priv.m_newKeysTyped, g_priv.m_newNumKeysTyped);
	g_priv.m_newNumKeysTyped = 0;

    // Count the number of key ups and downs this frame
    g_input.numKeyDowns = 0;
    g_input.numKeyUps = 0;
    for (int i = 0; i < KEY_MAX; ++i)
    {
        if (g_input.keyUps[i]) g_input.numKeyUps++;
        if (g_input.keyDowns[i]) g_input.numKeyDowns++;
    }

    g_input.lmb = g_priv.m_lmbPrivate;
    g_input.lmbClicked = g_input.lmb == true && g_priv.m_lmbOld == false;
	g_input.mmbClicked = g_input.mmb == true && g_priv.m_mmbOld == false;
	g_input.rmbClicked = g_input.rmb == true && g_priv.m_rmbOld == false;
	g_input.lmbUnClicked = g_input.lmb == false && g_priv.m_lmbOld == true;
	g_input.mmbUnClicked = g_input.mmb == false && g_priv.m_mmbOld == true;
	g_input.rmbUnClicked = g_input.rmb == false && g_priv.m_rmbOld == true;
	g_priv.m_lmbOld = g_input.lmb;
	g_priv.m_mmbOld = g_input.mmb;
	g_priv.m_rmbOld = g_input.rmb;

	g_input.lmbDoubleClicked = false;
	if (g_input.lmbClicked)
	{
 		double timeNow = GetRealTime();
		double delta = timeNow - g_priv.m_lastClickTime;
		if (delta < 0.25)
		{
			g_input.lmbDoubleClicked = true;
			g_input.lmbClicked = false;
		}
		g_priv.m_lastClickTime = timeNow;
		g_priv.m_lmbRepeatsSoFar = 0;
	}

	// Generate fake mouse up event(s) to compensate for the fact that Windows won't send
	// them if the last mouse down event was in the client area.
	if (g_priv.m_lastClickWasNC)
	{
		g_priv.m_lastClickWasNC = false;
		g_priv.m_lmbPrivate = false;
		g_input.mmb = false;
		g_input.rmb = false;
	}

	g_input.mouseVelX = g_input.mouseX - g_priv.m_mouseOldX;
	g_input.mouseVelY = g_input.mouseY - g_priv.m_mouseOldY;
	g_input.mouseVelZ = g_input.mouseZ - g_priv.m_mouseOldZ;
	g_priv.m_mouseOldX = g_input.mouseX;
	g_priv.m_mouseOldY = g_input.mouseY;
	g_priv.m_mouseOldZ = g_input.mouseZ;

    bool rv = g_input.eventSinceAdvance;
    g_input.eventSinceAdvance = false;
    return rv;
}


char const *GetKeyName(int i)
{
	static char name[16];
	name[0] = '\0';

	if (isdigit(i) || (isalpha(i) && isupper(i)))
	{
		name[0] = i;
		name[1] = '\0';
		return name;
	}

	// Keypad numbers
	if (i >= 96 && i <= 105)
	{
		strcat(name, "Keypad ");
		name[6] = i - 48;
		name[7] = '\0';
		return name;
	}

	// F1 through F10
	if (i >= 112 && i <= 123)
	{
		name[0] = 'F';
		if (i <= 120)
		{
			name[1] = i - 63,
			name[2] = '\0';
		}
		else
		{
			name[1] = '1';
			name[2] = i - 73;
			name[3] = '\0';
		}

		return name;
	}

	// All other named keys
	switch(i)
	{
		case 8:   return "Backspace";
		case 9:   return "Tab";
		case 13:  return "Enter";
		case 16:  return "Shift";
		case 17:  return "Ctrl";
		case 18:  return "Alt";
		case 19:  return "Pause";
		case 20:  return "Capslock";
		case 27:  return "Esc";
		case 32:  return "Space";
		case 33:  return "PgUp";
		case 34:  return "PgDn";
		case 35:  return "End";
		case 36:  return "Home";
		case 37:  return "Left";
		case 38:  return "Up";
		case 39:  return "Right";
		case 40:  return "Down";
		case 45:  return "Insert";
		case 46:  return "Del";
		case 106: return "*";
		case 107: return "Keypad +";
		case 109: return "Keypad -";
		case 110: return "Keypad Del";
		case 111: return "Keypad Slash";
		case 144: return "NumLock";
		case 145: return "ScrLock";
		case 186: return ":";
		case 187: return "=";
		case 188: return ",";
		case 189: return "-";
		case 190: return ".";
		case 191: return "/";
		case 192: return "\"";
		case 219: return "[";
		case 220: return "\\";
		case 221: return "]";
		case 223: return "~";
	}

	// Unnamed keys just use the ASCII value, printed in decimal
    strcpy(name, "unnamed");
    char *e = itoa(i, name + 7, 10);
    *e = '\0';

	return name;
}


int GetKeyId(char const *name)
{
	for (int i = 0; i < KEY_MAX; ++i)
	{
		char const *keyName = GetKeyName(i);
		if (keyName && strcasecmp(name, keyName) == 0)
		{
			return i;
		}
	}

	if (strncmp("unnamed", name, 7) == 0)
		return atoi(name + 7);

	return -1;
}


bool UntypeKey(char key)
{
    for (int i = 0; i < g_input.numKeysTyped; ++i)
    {
        if (g_input.keysTyped[i] == key)
        {
            g_input.keysTyped[i] = g_input.keysTyped[g_input.numKeysTyped - 1];
            g_input.numKeysTyped--;
            return true;
        }
    }

    return false;
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

    HDC dc = GetDC(g_hWnd);

    SetDIBitsToDevice(dc,
        0, 0, bmp->width, bmp->height,
        0, 0, 0, bmp->height,
        bmp->pixels, &binfo, DIB_RGB_COLORS
    );

    ReleaseDC(g_hWnd, dc);
}


void UpdateWin()
{
    // *** FPS Meter ***

    g_framesThisSecond++;

    double currentTime = GetRealTime();
    if (currentTime > g_endOfSecond)
    {
        // If program has been paused by a debugger, skip the seconds we missed
        if (currentTime > g_endOfSecond + 2.0)
            g_endOfSecond = currentTime + 1.0;
        else
            g_endOfSecond += 1.0;
        g_window->fps = g_framesThisSecond;
        g_framesThisSecond = 0;
    }
    else if (g_endOfSecond > currentTime + 2.0)
    {
        g_endOfSecond = currentTime + 1.0;
    }


    // *** Advance time ***

    g_window->advanceTime = currentTime - g_lastUpdateTime;
    g_lastUpdateTime = currentTime;


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
    if (g_dwmFlush) {
        g_dwmFlush();
        return true;
    }

    return false;
}

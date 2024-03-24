// System headers.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")
#include <shellapi.h>
#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore") // Needed for SetProcessDpiAwareness

// Standard headers.
#include <memory.h>


// Prototypes of Win API functions that we get via GetProcAddress().
typedef int (__stdcall EnableNonClientDpiScalingFunc)(HWND hwnd);


struct WindowPlatformSpecific {
    HWND hWnd;
    MouseCursorType currentMouseCursorType;
};


static bool g_funcPointersInitialized = false;
static EnableNonClientDpiScalingFunc *g_enabledNonClientDpiScalingFunc = NULL;


// ***************************************************************************
// Horrible mechanism to map HWNDs to DfWindows. Needed so that WndProc() can
// figure out which DfWindow() it is associated with. This wouldn't be needed
// if we could pass our DfWindow pointer into CreateWindow() as some void*
// data that it would give to every WndProc() call. But MS didn't think of that.
// Worse still, by the time CreateWindow() returns the HWND, WndProc() will
// already have been called. So we need to have stored the mapping before
// CreateWindow() returns. We do this by taking advantage of the fact that
// the initial call of WndProc() is guaranteed to be on the same thread that
// called CreateWindow(). We use a thread local to store the DfWindow we're
// in the process of making, before we call CreateWindow(). We check that in
// every call of GetWindowFromHwnd(), which is called at the start of every
// call of WndProc(). If the thread local DfWindow pointer it is set, we unset
// it and add the HWND->DfWindow mapping.

static DfWindow *g_newWindow = NULL;
enum { MAX_NUM_WINDOWS = 8 };
static DfWindow *g_windows[MAX_NUM_WINDOWS] = { 0 };

static DfWindow *GetWindowFromHWnd(HWND hWnd) {
    if (g_newWindow) {
        for (int i = 0; i < MAX_NUM_WINDOWS; i++) {
            if (g_windows[i] == NULL) {
                g_windows[i] = g_newWindow;
                g_windows[i]->_private->platSpec->hWnd = hWnd;
                g_newWindow = NULL;
                return g_windows[i];
            }
        }

        ReleaseAssert(0, "Too many windows");
    }

    for (int i = 0; i < MAX_NUM_WINDOWS; i++) {
        if (g_windows[i] && g_windows[i]->_private->platSpec->hWnd == hWnd)
            return g_windows[i];
    }

    return NULL;
}


static void RemoveHwnd(HWND hWnd) {
    for (int i = 0; i < MAX_NUM_WINDOWS; i++) {
        if (g_windows[i] && g_windows[i]->_private->platSpec->hWnd == hWnd)
            g_windows[i] = NULL;
    }
}

// End of horrible mechanism.
// ***************************************************************************


// Returns 0 if the event is handled here, -1 otherwise
static int EventHandler(DfWindow *win, unsigned int message, unsigned int wParam, int lParam) {
    static char const *s_keypressOutOfRangeMsg = "Keypress value out of range (%s: wParam = %d)";

    switch (message) {
        case WM_SYSCHAR:
            // We get one of these if the user presses alt+<regular key>. I don't understand exactly
            // what is going on here. If we don't handle it, we will then get a WM_SYSCOMMAND
            // message later and if we don't handle that, the system's default handler will
            // play the "Asterisk" sound which we don't want. If we do handle it and return 0,
            // everything is fine. BUT we shouldn't handle alt+space (ie when wParam == 32) because
            // that prevents the standard restore-move-size window menu from appearing (that
            // behaviour is part of the standard Windows window manager).
            if (wParam != 32) {
                break;
            }
            else {
                return -1;
            }

        case WM_SETFOCUS:
            HandleFocusInEvent(win);
            return -1;
        case WM_KILLFOCUS:
            HandleFocusOutEvent(win);
            break;

        case WM_CHAR:
            if (win->input.numKeysTyped < MAX_KEYS_TYPED_PER_FRAME &&
                !win->input.keys[KEY_CONTROL] &&
                !win->input.keys[KEY_ALT] &&
                wParam != KEY_ESC) {
                ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_CHAR", wParam);
                win->_private->newKeysTyped[win->_private->newNumKeysTyped] = wParam;
                win->_private->newNumKeysTyped++;
            }
            break;

        case WM_LBUTTONDOWN:
            win->_private->lmbPrivate = true;
            break;
        case WM_LBUTTONUP:
            win->_private->lmbPrivate = false;
            break;
        case WM_MBUTTONDOWN:
            win->input.mmb = true;
            break;
        case WM_MBUTTONUP:
            win->input.mmb = false;
            break;
        case WM_RBUTTONDOWN:
            win->input.rmb = true;
            break;
        case WM_RBUTTONUP:
            win->input.rmb = false;
            break;

        /* Mouse clicks in the Non-client area (ie titlebar) of the window are weird. If we
           handle them and return 0, then Windows ignores them and it is no longer possible
           to drag the window by the title bar. If we handle them and return -1, then Windows
           never sends us the button up event. I've chosen the second option and fix some of
           the brokenness by generating a fake up event one frame after the down event. */
        case WM_NCLBUTTONDOWN:
            win->_private->lmbPrivate = true;
            win->_private->lastClickWasNC = true;
            return -1;
        case WM_NCMBUTTONDOWN:
            win->input.mmb = true;
            win->_private->lastClickWasNC = true;
            return -1;
        case WM_NCRBUTTONDOWN:
            win->input.rmb = true;
            win->_private->lastClickWasNC = true;
            return -1;

        case WM_MOUSEWHEEL:
            win->input.mouseZ += (short)HIWORD(wParam);
            break;

        case WM_SYSKEYUP:
            ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_SYSKEYUP", wParam);

            // If the key event is the Control part of Alt_Gr, throw it away. There
            // will be an event for the alt part too.
            if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
                break;

            win->input.keys[wParam] = 0;
            win->_private->newKeyUps[wParam] = 1;
            break;

        case WM_SYSKEYDOWN:
        {
            ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_SYSKEYDOWN", wParam);
            win->input.keys[wParam] = 1;
            win->_private->newKeyDowns[wParam] = 1;
            break;
        }

        case WM_KEYUP:
        {
            // Alt key ups are presented here when the user keys, for example, Alt+F.
            // Windows will generate a SYSKEYUP event for the release of F, and a
            // normal KEYUP event for the release of the ALT. Very strange.
            ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_KEYUP", wParam);
            win->_private->newKeyUps[wParam] = 1;
            win->input.keys[wParam] = 0;
            break;
        }

        case WM_KEYDOWN:
        {
            ReleaseAssert(wParam < KEY_MAX, s_keypressOutOfRangeMsg, "WM_KEYDOWN", wParam);

            // The Windows key code for the DELETE key is 46. This is also the ASCII value of '.'
            // This is a problem because we need to put delete and backspace keypresses into the
            // typed text array (so that they are processed in the correct order with other
            // typed text). So, we convert 46 into KEY_DEL (127).
            if (wParam == 46)
                wParam = KEY_DEL;

            if (wParam == KEY_DEL) {
                win->_private->newKeysTyped[win->input.numKeysTyped] = KEY_DEL;
                win->_private->newNumKeysTyped++;
            }
            else if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0) {
                // Key event is the Control part of Alt_Gr, so throw it away. There
                // will be an event for the alt part too.
                break;
            }
            win->_private->newKeyDowns[wParam] = 1;
            win->input.keys[wParam] = 1;
            break;
        }

        // If the keyboard shortcut alt+space is pressed, Windows will bring up
        // a system menu that is part of the system provided window decoration.
        // We will see the alt down keypress but not the alt up because the
        // window system takes over our event loop while the menu is displayed.
        // We're sent WM_EXITMENULOOP when our event loop re-gains control. We
        // need to clear the alt down state.
        case WM_EXITMENULOOP:
            win->input.keys[KEY_ALT] = 0;
            break;

        case WM_MOUSEMOVE:
            break;

        default:
            return -1;
    }

    win->input.eventSinceAdvance = true;

    return 0;
}


bool InputPoll(DfWindow *win) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        // handle or dispatch messages
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // We request the mouse position every InputPoll instead of listening
    // for WM_MOUSEMOVE events because Windows doesn't send us those once
    // the cursor is outside the client rectangle. Using GetCursorPos()
    // gives us correct and up-to-date cursor position info all the time.
    POINT point;
    if (GetCursorPos(&point) != 0) {
        if (ScreenToClient(win->_private->platSpec->hWnd, &point)) {
            win->input.mouseX = point.x;
            win->input.mouseY = point.y;
        }
    }

    InputPollInternal(win);

    bool rv = win->input.eventSinceAdvance;
    win->input.eventSinceAdvance = false;
    return rv;
}



// ****************************************************************************
// Window Proc
// ****************************************************************************

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    DfWindow *win = GetWindowFromHWnd(hWnd);
    if (!win) goto _default;

    if (message == WM_SYSKEYDOWN && wParam == 115)
        SendMessage(hWnd, WM_CLOSE, 0, 0);
    switch (message) {
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

    switch (message) {
// TODO. Handle WM_SYSCOMMAND, and implement own window resizing code.
// Remove resize callback.
// Remove all the mouse button msg handlers and
// set/release capture stuff and use GetAsyncKeyState(VK_LBUTTON) instead.
// Merge the two WndProc functions.
//         case WM_SYSCOMMAND:
//             if ((wParam & 0xfff0) == SC_SIZE) {
//                 GetWindowRect(win->_private->platSpec->hWnd, &g_rect);
//                 xx = win->input.mouseX;
//                 yy = win->input.mouseY;
//                 moving = 1;
//                 return 0;
//             }
//             return DefWindowProc(hWnd, message, wParam, lParam);
        case WM_CREATE:
            if (g_enabledNonClientDpiScalingFunc)
                g_enabledNonClientDpiScalingFunc(win->_private->platSpec->hWnd);
            break;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                // Ignore set cursor messages, otherwise Windows will restore the
                // cursor to the arrow bitmap every time the mouse moves. We must
                // only do this if the mouse is in our client area, otherwise we
                // will prevent things like the Window Manager setting the correct
                // resize cursor when the user attempts to resize the window by
                // dragging the frame. We know the cursor is in our client area,
                // rather than someone else's, because this event is only sent if
                // the cursor is over our window.
                SetMouseCursor(win, win->_private->platSpec->currentMouseCursorType);
                break;
            }
            goto _default; // Pretend we didn't handle this message.

        case WM_DROPFILES:
            if (win->fileDropCallback) {
                HDROP drop = (HDROP)wParam;
                int numFiles = DragQueryFile(drop, -1, NULL, 0);
                for (int i = 0; i < numFiles; ++i) {
                    char buf[MAX_PATH + 1];
                    DragQueryFile(drop, i, buf, MAX_PATH);
                    win->fileDropCallback(buf);
                }

                win->input.eventSinceAdvance = true;
            }
            break;

        case WM_WINDOWPOSCHANGED:
        {
            WINDOWPOS *wp = (WINDOWPOS *)lParam;
            win->left = wp->x;
            win->top = wp->y;
            win->input.eventSinceAdvance = true;
            goto _default; // Pretend we didn't handle this message.
        }

        case WM_SIZE:
        {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            if (win->bmp->width != w || win->bmp->height != h) {
                BitmapDelete(win->bmp);
                win->bmp = BitmapCreate(w, h);
                if (win->redrawCallback) {
                    win->redrawCallback();
                }
                win->input.eventSinceAdvance = true;
            }
            break;
        }

        case WM_CLOSE:
            if (win)
                win->windowClosed = true;
            win->input.eventSinceAdvance = true;
            return 0;

        _default:
        default:
            if (!win || EventHandler(win, message, wParam, lParam) == -1)
                return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}


bool GetDesktopRes(int *width, int *height) {
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


DfWindow *CreateWin(int width, int height, WindowType winType, char const *winName) {
    return CreateWinPos(CW_USEDEFAULT, CW_USEDEFAULT, width, height, winType, winName);
}


DfWindow *CreateWinPos(int x, int y, int width, int height, WindowType winType, char const *winName) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    DfWindow *win = new DfWindow;
    memset(win, 0, sizeof(DfWindow));
    win->_private = new DfWindowPrivate;
    memset(win->_private, 0, sizeof(DfWindowPrivate));
    win->_private->platSpec = new WindowPlatformSpecific;
    win->_private->platSpec->currentMouseCursorType = MCT_ARROW;

    g_newWindow = win;

    width = ClampInt(width, 100, 4000);
    height = ClampInt(height, 100, 4000);

    // Register window class
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = winName;
    RegisterClass(&wc);

    win->bmp = BitmapCreate(width, height);

    unsigned windowStyle = WS_VISIBLE;
    if (winType == WT_FULLSCREEN) {
        windowStyle |= WS_POPUP;

        DEVMODE devmode;
        devmode.dmSize = sizeof(DEVMODE);
        devmode.dmBitsPerPel = 32;
        devmode.dmPelsWidth = width;
        devmode.dmPelsHeight = height;
        devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        ReleaseAssert(ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL,
            "Couldn't change screen resolution to %i x %i", width, height);
    }
    else {
        if (winType == WT_WINDOWED_RESIZEABLE)
            windowStyle |= WS_OVERLAPPEDWINDOW;
        else
            windowStyle |= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;

        RECT windowRect = { 0, 0, width, height };
        AdjustWindowRect(&windowRect, windowStyle, false);
        int borderWidth = (windowRect.right - windowRect.left) - width;
        int titleHeight = ((windowRect.bottom - windowRect.top) - height) - borderWidth;
        width += borderWidth;
        height += borderWidth + titleHeight;
    }

    if (!g_funcPointersInitialized) {
        HMODULE user32 = LoadLibrary("user32.dll");
        g_enabledNonClientDpiScalingFunc = (EnableNonClientDpiScalingFunc*)
            GetProcAddress(user32, "EnableNonClientDpiScaling");
        g_funcPointersInitialized = true;
    }

    // Create main window.
    // We ignore the returned HWND because it will already have been stored
    // in the HWND->DfWindow mapping before this function returns.
    CreateWindow(wc.lpszClassName, wc.lpszClassName,
        windowStyle, x, y, width, height,
        NULL, NULL, 0, NULL);

    double now = GetRealTime();
    win->_private->lastUpdateTime = now;
    win->_private->endOfSecond = now + 1.0;

    // Register as a drag-and-drop target.
    DragAcceptFiles(win->_private->platSpec->hWnd, true);

    InitInput(win);

    return win;
}


void DestroyWin(DfWindow *win) {
    RemoveHwnd(win->_private->platSpec->hWnd);
    DestroyWindow(win->_private->platSpec->hWnd);
    BitmapDelete(win->bmp);
    delete win;
}


int GetMonitorDpi(DfWindow *win) {
    HMONITOR hmon = MonitorFromWindow(win->_private->platSpec->hWnd, MONITOR_DEFAULTTONEAREST);
    unsigned dpiX, dpiY;
    GetDpiForMonitor(hmon, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    return dpiX;
}


void GetMonitorWorkArea(DfWindow *win, int *x, int *y, int *width, int *height) {
    HMONITOR hmon = MonitorFromWindow(win->_private->platSpec->hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info;
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoA(hmon, &info);
    *x = info.rcWork.left;
    *y = info.rcWork.top;
    *width = info.rcWork.right - info.rcWork.left;
    *height = info.rcWork.bottom - info.rcWork.top;
}


void SetWindowRect(DfWindow *win, int x, int y, int width, int height) {
    RECT rectWithShadow;
    GetWindowRect(win->_private->platSpec->hWnd, &rectWithShadow);
    int withShadowWidth = rectWithShadow.right - rectWithShadow.left;
    int withShadowHeight = rectWithShadow.bottom - rectWithShadow.top;

    RECT rectWithoutShadow;
    DwmGetWindowAttribute(win->_private->platSpec->hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, 
        &rectWithoutShadow, sizeof(RECT));
    int withoutShadowWidth = rectWithoutShadow.right - rectWithoutShadow.left;
    int withoutShadowHeight = rectWithoutShadow.bottom - rectWithoutShadow.top;

    int shadowWidth = abs(withShadowWidth - withoutShadowWidth);
    int shadowHeight = abs(withShadowHeight - withoutShadowHeight);
    MoveWindow(win->_private->platSpec->hWnd, 
        x - shadowWidth/2, y, 
        width + shadowWidth, height + shadowHeight, false);
}


// This function copies a DfBitmap to the window, so you can actually see it.
// SetBIBitsToDevice seems to be the fastest way to achieve this on most hardware.
static void BlitBitmapToWindow(DfWindow *win) {
    DfBitmap *bmp = win->bmp;
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

    HDC dc = GetDC(win->_private->platSpec->hWnd);

    SetDIBitsToDevice(dc,
        0, 0, bmp->width, bmp->height,
        0, 0, 0, bmp->height,
        bmp->pixels, &binfo, DIB_RGB_COLORS
    );

    ReleaseDC(win->_private->platSpec->hWnd, dc);
}


void *GetWindowHandle(DfWindow *win) {
    return win->_private->platSpec->hWnd;
}


void ShowMouse(DfWindow *) {
    ShowCursor(true);
}


void HideMouse(DfWindow *) {
    ShowCursor(false);
}


void SetMouseCursor(DfWindow *win, MouseCursorType t) {
    // Ignore the request if the cursor is outside the client rectangle because
    // otherwise if your app sets the mouse cursor repeatedly, in the main loop
    // say, then this would clash with the Windows' window manager setting the
    // resizing cursor when the mouse was in the non-client rectangle.
    if (win->input.mouseX < 0 ||
        win->input.mouseX >= win->bmp->width ||
        win->input.mouseY < 0 ||
        win->input.mouseY >= win->bmp->height) {
        return;
    }

    switch (t) {
    case MCT_ARROW:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        break;
    case MCT_IBEAM:
        SetCursor(LoadCursor(NULL, IDC_IBEAM));
        break;
    case MCT_RESIZE_WIDTH:
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        break;
    case MCT_RESIZE_HEIGHT:
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
        break;
    }

    win->_private->platSpec->currentMouseCursorType = t;
}


void SetWindowIcon(DfWindow *win) {
    HINSTANCE hInstance = GetModuleHandle(0);
    HANDLE hIcon = LoadImageA(hInstance, "#101", IMAGE_ICON, 16, 16, 0);
    if (hIcon) {
        SendMessage(win->_private->platSpec->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(win->_private->platSpec->hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
    hIcon = LoadImageA(hInstance, "#101", IMAGE_ICON, 32, 32, 0);
    if (hIcon) {
        SendMessage(win->_private->platSpec->hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }
}


bool IsWindowMaximized(DfWindow *win) {
    WINDOWPLACEMENT winPlacement;
    winPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(win->_private->platSpec->hWnd, &winPlacement);
    return winPlacement.showCmd == SW_SHOWMAXIMIZED;
}


void SetMaximizedState(DfWindow *win, bool maximize) {
    if (maximize) {
        ShowWindow(win->_private->platSpec->hWnd, SW_MAXIMIZE);
    }
    else {
        ShowWindow(win->_private->platSpec->hWnd, SW_RESTORE);
    }
}


void BringWindowToFront(DfWindow *win) {
    BringWindowToTop(win->_private->platSpec->hWnd);
    SetForegroundWindow(win->_private->platSpec->hWnd);
}


void SetWindowTitle(DfWindow *win, char const *title) {
    SetWindowText(win->_private->platSpec->hWnd, title);
}


bool WaitVsync() {
    DwmFlush();
    return true;
}

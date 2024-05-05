// Own header
#include "df_window.h"

// Project headers
#include "df_bitmap.h"
#include "df_time.h"

// Standard includes
#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


struct WindowPlatformSpecific;


struct _DfWindowPrivate {
    bool        lastClickWasNC;       // True if mouse was outside the client area of the window when the button was last clicked
    double      lastClickTime;        // Used in double click detection
    int         lmbRepeatsSoFar;      // When lmb is held, this value is used by IsLmbRepeatSet to determine when to return true

    int         mouseOldX;            // Used to calculate input.mouseVelX
    int         mouseOldY;
    int         mouseOldZ;

    signed char newKeyDowns[KEY_MAX];
    signed char newKeyUps[KEY_MAX];
    char        newKeysTyped[MAX_KEYS_TYPED_PER_FRAME];
    int         newNumKeysTyped;

    unsigned    framesThisSecond;
    double      endOfSecond;
    double      lastUpdateTime;

    WindowPlatformSpecific *platSpec;
};


DfWindow *g_window = NULL;


static void InitInput(DfWindow *);
static void InputPollInternal(DfWindow *win);


static void HandleFocusInEvent(DfWindow *win) {
    win->input.windowHasFocus = true;
    memset(win->_private->newKeyDowns, 0, KEY_MAX);
    memset(win->_private->newKeyUps, 0, KEY_MAX);
    memset(win->input.keys, 0, KEY_MAX);
}


static void HandleFocusOutEvent(DfWindow *win) {
    win->input.windowHasFocus = false;
}


#ifdef _WIN32
# include "df_window_windows.cpp"
#else
# include "df_window_x11.cpp"
#endif


static void InitInput(DfWindow *win) {
    win->input.eventSinceAdvance = true;
    win->input.windowHasFocus = true;
}


char const *GetKeyName(int i) {
    static char name[16];
    name[0] = '\0';

    if (isdigit(i) || (isalpha(i) && isupper(i))) {
        name[0] = i;
        name[1] = '\0';
        return name;
    }

    // Keypad numbers
    if (i >= 96 && i <= 105) {
        strcat(name, "Keypad ");
        name[6] = i - 48;
        name[7] = '\0';
        return name;
    }

    // F1 through F10
    if (i >= 112 && i <= 123) {
        name[0] = 'F';
        if (i <= 120) {
            name[1] = i - 63,
            name[2] = '\0';
        }
        else {
            name[1] = '1';
            name[2] = i - 73;
            name[3] = '\0';
        }

        return name;
    }

    // All other named keys
    switch (i) {
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
        case 106: return "*";
        case 107: return "KeypadPlus";
        case 109: return "KeypadMinus";
        case 110: return "KeypadDel";
        case 111: return "KeypadSlash";
        case 127: return "Del";
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

    // Unnamed keys just use the ASCII value, printed in hex.
    memcpy(name, "unnamed", 7);
    name[7] = (i >> 4) & 0xff;
    name[8] = i & 0xff;
    name[9] = '\0';

    return name;
}


int GetKeyId(char const *name) {
    for (int i = 0; i < KEY_MAX; ++i) {
        char const *keyName = GetKeyName(i);
        if (keyName && strcasecmp(name, keyName) == 0) {
            return i;
        }
    }

    if (strncmp("unnamed", name, 7) == 0)
        return atoi(name + 7);

    return -1;
}


bool UntypeKey(DfWindow *win, char key) {
    for (int i = 0; i < win->input.numKeysTyped; ++i) {
        if (win->input.keysTyped[i] == key) {
            win->input.keysTyped[i] = win->input.keysTyped[win->input.numKeysTyped - 1];
            win->input.numKeysTyped--;
            return true;
        }
    }

    return false;
}


static void InputPollInternal(DfWindow *win)
{
    memcpy(win->input.keyDowns, win->_private->newKeyDowns, KEY_MAX);
    memset(win->_private->newKeyDowns, 0, KEY_MAX);
    memcpy(win->input.keyUps, win->_private->newKeyUps, KEY_MAX);
    memset(win->_private->newKeyUps, 0, KEY_MAX);

    win->input.numKeysTyped = win->_private->newNumKeysTyped;
    memcpy(win->input.keysTyped, win->_private->newKeysTyped, win->_private->newNumKeysTyped);
    win->_private->newNumKeysTyped = 0;

    // Count the number of key ups and downs this frame
    win->input.numKeyDowns = 0;
    win->input.numKeyUps = 0;
    for (int i = 0; i < KEY_MAX; ++i) {
        if (win->input.keyUps[i]) win->input.numKeyUps++;
        if (win->input.keyDowns[i]) win->input.numKeyDowns++;
    }

    win->input.lmbDoubleClicked = false;
    if (win->input.lmbClicked) {
        double timeNow = GetRealTime();
        double delta = timeNow - win->_private->lastClickTime;
        if (delta < 0.25) {
            win->input.lmbDoubleClicked = true;
            win->input.lmbClicked = false;
        }
        win->_private->lastClickTime = timeNow;
        win->_private->lmbRepeatsSoFar = 0;
    }

    // Generate fake mouse up event(s) to compensate for the fact that Windows won't send
    // them if the last mouse down event was in the client area.
    if (win->_private->lastClickWasNC) {
        win->_private->lastClickWasNC = false;
        win->input.lmb = false;
        win->input.mmb = false;
        win->input.rmb = false;
    }

    win->input.mouseVelX = win->input.mouseX - win->_private->mouseOldX;
    win->input.mouseVelY = win->input.mouseY - win->_private->mouseOldY;
    win->input.mouseVelZ = win->input.mouseZ - win->_private->mouseOldZ;
    win->_private->mouseOldX = win->input.mouseX;
    win->_private->mouseOldY = win->input.mouseY;
    win->_private->mouseOldZ = win->input.mouseZ;
}


void UpdateWin(DfWindow *win) {
    // *** FPS Meter ***

    win->_private->framesThisSecond++;

    double currentTime = GetRealTime();
    if (currentTime > win->_private->endOfSecond) {
        // If program has been paused by a debugger, skip the seconds we missed
        if (currentTime > win->_private->endOfSecond + 2.0)
            win->_private->endOfSecond = currentTime + 1.0;
        else
            win->_private->endOfSecond += 1.0;
        win->fps = win->_private->framesThisSecond;
        win->_private->framesThisSecond = 0;
    }
    else if (win->_private->endOfSecond > currentTime + 2.0) {
        win->_private->endOfSecond = currentTime + 1.0;
    }


    // *** Advance time ***

    win->advanceTime = currentTime - win->_private->lastUpdateTime;
    win->_private->lastUpdateTime = currentTime;


    // *** Swap buffers ***

    BlitBitmapToWindow(win);
}


void RegisterRedrawCallback(DfWindow *win, RedrawCallback *proc) {
    win->redrawCallback = proc;
}

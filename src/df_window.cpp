// Own header
#include "df_window.h"

// Project headers
#include "df_bitmap.h"
#include "df_time.h"

// Standard includes
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>


struct InputPrivate
{
    // TODO: Consider removing the m_ prefixes.
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

    signed char	m_newKeyDowns[KEY_MAX];
    signed char	m_newKeyUps[KEY_MAX];
    char		m_newKeysTyped[MAX_KEYS_TYPED_PER_FRAME];
    int			m_newNumKeysTyped;
};


static unsigned g_framesThisSecond = 0;
static double g_endOfSecond = 0.0;
static double g_lastUpdateTime = 0.0;
static InputPrivate g_priv = { 0 };

DfInput g_input = { 0 };
DfWindow *g_window = NULL;


static void InitInput();
static void InputPollInternal();


#ifdef WIN32
# include "df_window_windows.cpp"
#else
# include "df_window_x11.cpp"
#endif


static void InitInput()
{
    g_input.eventSinceAdvance = true;
    g_input.windowHasFocus = true;
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

	// Unnamed keys just use the ASCII value, printed in decimal
    sprintf(name, "unnamed%i", i);

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


static void InputPollInternal()
{
    memcpy(g_input.keyDowns, g_priv.m_newKeyDowns, KEY_MAX);
    memset(g_priv.m_newKeyDowns, 0, KEY_MAX);
    memcpy(g_input.keyUps, g_priv.m_newKeyUps, KEY_MAX);
    memset(g_priv.m_newKeyUps, 0, KEY_MAX);

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

    BlitBitmapToWindow(g_window);
}


void RegisterRedrawCallback(RedrawCallback *proc)
{
    g_window->redrawCallback = proc;
}

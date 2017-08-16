#include "df_input.h"

#include "df_time.h"
#include "df_window.h"

#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>


DfInput g_input;


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


void CreateInputManager()
{
    memset(&g_input, 0, sizeof(DfInput));

    g_input.priv = new InputPrivate;
    memset(g_input.priv, 0, sizeof(InputPrivate));
	g_input.priv->m_lastClickTime = 0.0;

    g_input.eventSinceAdvance = true;
    g_input.windowHasFocus = true;
}


// Returns 0 if the event is handled here, -1 otherwise
int EventHandler(unsigned int message, unsigned int wParam, int lParam, bool /*_isAReplayedEvent*/)
{
	if (!g_input.priv)
		return -1;

	switch (message)
	{
        case WM_ACTIVATE:
            break;
		case WM_SETFOCUS:
		case WM_NCACTIVATE:
			g_input.windowHasFocus = true;
			// Clear keyboard state when we regain focus
            memset(g_input.priv->m_keyNewDowns, 0, KEY_MAX);
            memset(g_input.priv->m_keyNewUps, 0, KEY_MAX);
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
				ReleaseAssert(wParam < KEY_MAX,
					"Keypress value out of range (WM_CHAR: wParam = %d)", wParam);
				g_input.priv->m_newKeysTyped[g_input.priv->m_newNumKeysTyped] = wParam;
				g_input.priv->m_newNumKeysTyped++;
			}
			break;

		case WM_LBUTTONDOWN:
			g_input.priv->m_lmbPrivate = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_LBUTTONUP:
			g_input.priv->m_lmbPrivate = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_MBUTTONDOWN:
			g_input.mmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_MBUTTONUP:
			g_input.mmb = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_RBUTTONDOWN:
			g_input.rmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_RBUTTONUP:
			g_input.rmb = false;
//			g_windowManager->UncaptureMouse();
			break;

		/* Mouse clicks in the Non-client area (ie titlebar) of the window are weird. If we
		   handle them and return 0, then Windows ignores them and it is no longer possible
		   to drag the window by the title bar. If we handle them and return -1, then Windows
		   never sends us the button up event. I've chosen the second option and fix some of
		   the brokenness by generating a fake up event one frame after the down event. */
		case WM_NCLBUTTONDOWN:
			g_input.priv->m_lmbPrivate = true;
			g_input.priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCMBUTTONDOWN:
			g_input.mmb = true;
			g_input.priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCRBUTTONDOWN:
			g_input.rmb = true;
			g_input.priv->m_lastClickWasNC = true;
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
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYUP: wParam = %d)", wParam);

            // If the key event is the Control part of Alt_Gr, throw it away. There
            // will be an event for the alt part too.
            if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
                break;

            g_input.keys[wParam] = 0;
            g_input.priv->m_keyNewUps[wParam] = 1;
            break;

		case WM_SYSKEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYDOWN: wParam = %d)", wParam);
			//int flags = (short)HIWORD(lParam);
			g_input.keys[wParam] = 1;
			g_input.priv->m_keyNewDowns[wParam] = 1;
			break;
		}

		case WM_KEYUP:
		{
			// Alt key ups are presented here when the user keys, for example, Alt+F.
			// Windows will generate a SYSKEYUP event for the release of F, and a
			// normal KEYUP event for the release of the ALT. Very strange.
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYUP: wParam = %d)", wParam);
            g_input.priv->m_keyNewUps[wParam] = 1;
            g_input.keys[wParam] = 0;
			break;
		}

		case WM_KEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYDOWN: wParam = %d)", wParam);
			if (wParam == KEY_DEL)
			{
				g_input.priv->m_newKeysTyped[g_input.numKeysTyped] = 127;
				g_input.priv->m_newNumKeysTyped++;
			}
            else if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
            {
                //int res = GetAsyncKeyState(VK_MENU);
                // Key event is the Control part of Alt_Gr, so throw it away. There
                // will be an event for the alt part too.
                break;
            }
			g_input.priv->m_keyNewDowns[wParam] = 1;
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

    memcpy(g_input.keyDowns, g_input.priv->m_keyNewDowns, KEY_MAX);
    memset(g_input.priv->m_keyNewDowns, 0, KEY_MAX);
    memcpy(g_input.keyUps, g_input.priv->m_keyNewUps, KEY_MAX);
    memset(g_input.priv->m_keyNewUps, 0, KEY_MAX);

	g_input.numKeysTyped = g_input.priv->m_newNumKeysTyped;
	memcpy(g_input.keysTyped, g_input.priv->m_newKeysTyped, g_input.priv->m_newNumKeysTyped);
	g_input.priv->m_newNumKeysTyped = 0;

    // Count the number of key ups and downs this frame
    g_input.numKeyDowns = 0;
    g_input.numKeyUps = 0;
    for (int i = 0; i < KEY_MAX; ++i)
    {
        if (g_input.keyUps[i]) g_input.numKeyUps++;
        if (g_input.keyDowns[i]) g_input.numKeyDowns++;
    }

    g_input.lmb = g_input.priv->m_lmbPrivate;
    g_input.lmbClicked = g_input.lmb == true && g_input.priv->m_lmbOld == false;
	g_input.mmbClicked = g_input.mmb == true && g_input.priv->m_mmbOld == false;
	g_input.rmbClicked = g_input.rmb == true && g_input.priv->m_rmbOld == false;
	g_input.lmbUnClicked = g_input.lmb == false && g_input.priv->m_lmbOld == true;
	g_input.mmbUnClicked = g_input.mmb == false && g_input.priv->m_mmbOld == true;
	g_input.rmbUnClicked = g_input.rmb == false && g_input.priv->m_rmbOld == true;
	g_input.priv->m_lmbOld = g_input.lmb;
	g_input.priv->m_mmbOld = g_input.mmb;
	g_input.priv->m_rmbOld = g_input.rmb;

	g_input.lmbDoubleClicked = false;
	if (g_input.lmbClicked)
	{
 		double timeNow = DfGetTime();
		double delta = timeNow - g_input.priv->m_lastClickTime;
		if (delta < 0.25)
		{
			g_input.lmbDoubleClicked = true;
			g_input.lmbClicked = false;
		}
		g_input.priv->m_lastClickTime = timeNow;
		g_input.priv->m_lmbRepeatsSoFar = 0;
	}

	// Generate fake mouse up event(s) to compensate for the fact that Windows won't send
	// them if the last mouse down event was in the client area.
	if (g_input.priv->m_lastClickWasNC)
	{
		g_input.priv->m_lastClickWasNC = false;
		g_input.priv->m_lmbPrivate = false;
		g_input.mmb = false;
		g_input.rmb = false;
	}

	g_input.mouseVelX = g_input.mouseX - g_input.priv->m_mouseOldX;
	g_input.mouseVelY = g_input.mouseY - g_input.priv->m_mouseOldY;
	g_input.mouseVelZ = g_input.mouseZ - g_input.priv->m_mouseOldZ;
	g_input.priv->m_mouseOldX = g_input.mouseX;
	g_input.priv->m_mouseOldY = g_input.mouseY;
	g_input.priv->m_mouseOldZ = g_input.mouseZ;

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

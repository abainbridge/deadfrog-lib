#include "df_input.h"

#include "df_hi_res_time.h"
#include "df_window_manager.h"

#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>


InputManager g_inputManager;


struct InputManagerPrivate
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
    memset(&g_inputManager, 0, sizeof(InputManager));

    g_inputManager.priv = new InputManagerPrivate;
    memset(g_inputManager.priv, 0, sizeof(InputManagerPrivate));
	g_inputManager.priv->m_lastClickTime = 0.0;

    g_inputManager.eventSinceAdvance = true;
    g_inputManager.windowHasFocus = true;
}


// Returns 0 if the event is handled here, -1 otherwise
int EventHandler(unsigned int message, unsigned int wParam, int lParam, bool /*_isAReplayedEvent*/)
{
	if (!g_inputManager.priv)
		return -1;

	switch (message)
	{
        case WM_ACTIVATE:
            break;
		case WM_SETFOCUS:
		case WM_NCACTIVATE:
			g_inputManager.windowHasFocus = true;
			// Clear keyboard state when we regain focus
            memset(g_inputManager.priv->m_keyNewDowns, 0, KEY_MAX);
            memset(g_inputManager.priv->m_keyNewUps, 0, KEY_MAX);
            memset(g_inputManager.keys, 0, KEY_MAX);
			return -1;
		case WM_KILLFOCUS:
			g_inputManager.windowHasFocus = false;
            g_inputManager.mouseX = -1;
            g_inputManager.mouseY = -1;
			break;

        case WM_SYSCHAR:
            break;

		case WM_CHAR:
			if (g_inputManager.numKeysTyped < MAX_KEYS_TYPED_PER_FRAME &&
				!g_inputManager.keys[KEY_CONTROL] &&
				!g_inputManager.keys[KEY_ALT] &&
				wParam != KEY_ESC)
			{
				ReleaseAssert(wParam < KEY_MAX,
					"Keypress value out of range (WM_CHAR: wParam = %d)", wParam);
				g_inputManager.priv->m_newKeysTyped[g_inputManager.priv->m_newNumKeysTyped] = wParam;
				g_inputManager.priv->m_newNumKeysTyped++;
			}
			break;

		case WM_LBUTTONDOWN:
			g_inputManager.priv->m_lmbPrivate = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_LBUTTONUP:
			g_inputManager.priv->m_lmbPrivate = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_MBUTTONDOWN:
			g_inputManager.mmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_MBUTTONUP:
			g_inputManager.mmb = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_RBUTTONDOWN:
			g_inputManager.rmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_RBUTTONUP:
			g_inputManager.rmb = false;
//			g_windowManager->UncaptureMouse();
			break;

		/* Mouse clicks in the Non-client area (ie titlebar) of the window are weird. If we
		   handle them and return 0, then Windows ignores them and it is no longer possible
		   to drag the window by the title bar. If we handle them and return -1, then Windows
		   never sends us the button up event. I've chosen the second option and fix some of
		   the brokenness by generating a fake up event one frame after the down event. */
		case WM_NCLBUTTONDOWN:
			g_inputManager.priv->m_lmbPrivate = true;
			g_inputManager.priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCMBUTTONDOWN:
			g_inputManager.mmb = true;
			g_inputManager.priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCRBUTTONDOWN:
			g_inputManager.rmb = true;
			g_inputManager.priv->m_lastClickWasNC = true;
			return -1;
			break;

		case WM_MOUSEWHEEL:
		{
			int move = (short)HIWORD(wParam) / 120;
			g_inputManager.mouseZ += move;
			break;
		}

		case WM_NCMOUSEMOVE:
			g_inputManager.mouseX = -1;
			g_inputManager.mouseY = -1;
			break;

		case WM_MOUSEMOVE:
		{
			short newPosX = lParam & 0xFFFF;
			short newPosY = short(lParam >> 16);
			g_inputManager.mouseX = newPosX;
			g_inputManager.mouseY = newPosY;
			break;
		}

		case WM_SYSKEYUP:
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYUP: wParam = %d)", wParam);

            // If the key event is the Control part of Alt_Gr, throw it away. There
            // will be an event for the alt part too.
            if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
                break;

            g_inputManager.keys[wParam] = 0;
            g_inputManager.priv->m_keyNewUps[wParam] = 1;
            break;

		case WM_SYSKEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYDOWN: wParam = %d)", wParam);
			//int flags = (short)HIWORD(lParam);
			g_inputManager.keys[wParam] = 1;
			g_inputManager.priv->m_keyNewDowns[wParam] = 1;
			break;
		}

		case WM_KEYUP:
		{
			// Alt key ups are presented here when the user keys, for example, Alt+F.
			// Windows will generate a SYSKEYUP event for the release of F, and a
			// normal KEYUP event for the release of the ALT. Very strange.
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYUP: wParam = %d)", wParam);
            g_inputManager.priv->m_keyNewUps[wParam] = 1;
            g_inputManager.keys[wParam] = 0;
			break;
		}

		case WM_KEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYDOWN: wParam = %d)", wParam);
			if (wParam == KEY_DEL)
			{
				g_inputManager.priv->m_newKeysTyped[g_inputManager.numKeysTyped] = 127;
				g_inputManager.priv->m_newNumKeysTyped++;
			}
            else if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
            {
                //int res = GetAsyncKeyState(VK_MENU);
                // Key event is the Control part of Alt_Gr, so throw it away. There
                // will be an event for the alt part too.
                break;
            }
			g_inputManager.priv->m_keyNewDowns[wParam] = 1;
			g_inputManager.keys[wParam] = 1;
			break;
		}

		default:
			return -1;
	}

    g_inputManager.eventSinceAdvance = true;
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

    memcpy(g_inputManager.keyDowns, g_inputManager.priv->m_keyNewDowns, KEY_MAX);
    memset(g_inputManager.priv->m_keyNewDowns, 0, KEY_MAX);
    memcpy(g_inputManager.keyUps, g_inputManager.priv->m_keyNewUps, KEY_MAX);
    memset(g_inputManager.priv->m_keyNewUps, 0, KEY_MAX);

	g_inputManager.numKeysTyped = g_inputManager.priv->m_newNumKeysTyped;
	memcpy(g_inputManager.keysTyped, g_inputManager.priv->m_newKeysTyped, g_inputManager.priv->m_newNumKeysTyped);
	g_inputManager.priv->m_newNumKeysTyped = 0;


    g_inputManager.lmb = g_inputManager.priv->m_lmbPrivate;
    if (g_inputManager.priv->m_lastClickWasNC)
        g_inputManager.lmbClicked = false;
	else
        g_inputManager.lmbClicked = g_inputManager.lmb == true && g_inputManager.priv->m_lmbOld == false;
	g_inputManager.mmbClicked = g_inputManager.mmb == true && g_inputManager.priv->m_mmbOld == false;
	g_inputManager.rmbClicked = g_inputManager.rmb == true && g_inputManager.priv->m_rmbOld == false;
	g_inputManager.lmbUnClicked = g_inputManager.lmb == false && g_inputManager.priv->m_lmbOld == true;
	g_inputManager.mmbUnClicked = g_inputManager.mmb == false && g_inputManager.priv->m_mmbOld == true;
	g_inputManager.rmbUnClicked = g_inputManager.rmb == false && g_inputManager.priv->m_rmbOld == true;
	g_inputManager.priv->m_lmbOld = g_inputManager.lmb;
	g_inputManager.priv->m_mmbOld = g_inputManager.mmb;
	g_inputManager.priv->m_rmbOld = g_inputManager.rmb;

	g_inputManager.lmbDoubleClicked = false;
	if (g_inputManager.lmbClicked)
	{
 		double timeNow = GetHighResTime();
		double delta = timeNow - g_inputManager.priv->m_lastClickTime;
		if (delta < 0.25)
		{
			g_inputManager.lmbDoubleClicked = true;
			g_inputManager.lmbClicked = false;
		}
		g_inputManager.priv->m_lastClickTime = timeNow;
		g_inputManager.priv->m_lmbRepeatsSoFar = 0;
	}

	// Generate fake mouse up event(s) to compensate for the fact that Windows won't send
	// them if the last mouse down event was in the client area.
	if (g_inputManager.priv->m_lastClickWasNC)
	{
		g_inputManager.priv->m_lastClickWasNC = false;
		g_inputManager.priv->m_lmbPrivate = false;
		g_inputManager.mmb = false;
		g_inputManager.rmb = false;
	}

	g_inputManager.mouseVelX = g_inputManager.mouseX - g_inputManager.priv->m_mouseOldX;
	g_inputManager.mouseVelY = g_inputManager.mouseY - g_inputManager.priv->m_mouseOldY;
	g_inputManager.mouseVelZ = g_inputManager.mouseZ - g_inputManager.priv->m_mouseOldZ;
	g_inputManager.priv->m_mouseOldX = g_inputManager.mouseX;
	g_inputManager.priv->m_mouseOldY = g_inputManager.mouseY;
	g_inputManager.priv->m_mouseOldZ = g_inputManager.mouseZ;

    bool rv = g_inputManager.eventSinceAdvance;
    g_inputManager.eventSinceAdvance = false;
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
	sprintf(name, "unnamed%d", i);
	return name;
}


int GetKeyId(char const *name)
{
	for (int i = 0; i < KEY_MAX; ++i)
	{
		char const *keyName = GetKeyName(i);
		if (keyName && stricmp(name, keyName) == 0)
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
    for (int i = 0; i < g_inputManager.numKeysTyped; ++i)
    {
        if (g_inputManager.keysTyped[i] == key)
        {
            g_inputManager.keysTyped[i] = g_inputManager.keysTyped[g_inputManager.numKeysTyped - 1];
            g_inputManager.numKeysTyped--;
            return true;
        }
    }

    return false;
}

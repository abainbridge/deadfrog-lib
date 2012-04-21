#include <ctype.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input.h"
#include "lib/window_manager.h"


#define LMB_REPEAT_PERIOD			0.07f	// Seconds


struct InputManagerPrivate
{
    bool        m_lmbPrivate;
    bool		m_lmbOld;				// Mouse button states from last frame. Only 
    bool		m_mmbOld;				// used to computer the deltas.
    bool		m_rmbOld;

    bool		m_lastClickWasNC;		// True if mouse was outside the client area of the window when the button was last clicked
    double		m_lastClickTime;		// Used in double click detection
    int			m_lmbRepeatsSoFar;		// When lmb is held, this value is used by IsLmbRepeatSet to determine when to return true

    bool		m_rawLmbClicked;

    int			m_mouseOldX;			// Same things but for position
    int			m_mouseOldY;
    int			m_mouseOldZ;

    signed char	m_keyNewDowns[KEY_MAX];
    signed char	m_keyNewUps[KEY_MAX];
    char		m_newKeysTyped[MAX_KEYS_TYPED_PER_FRAME];
    int			m_newNumKeysTyped;

    MouseUpdateCallback *m_mouseCallback;
};


InputManager *CreateInputManager()
{
    InputManager *im = new InputManager;
    im->m_priv = new InputManagerPrivate;

    im->m_priv->m_lmbPrivate = false;
    im->m_priv->m_lmbOld = false;
    im->m_priv->m_mmbOld = false;
    im->m_priv->m_rmbOld = false;
	im->m_priv->m_lastClickWasNC = false;
	im->m_priv->m_lastClickTime = 0.0;
	im->m_priv->m_lmbRepeatsSoFar = 0;
	im->m_priv->m_rawLmbClicked = false;
    im->m_priv->m_mouseOldX = 0;
    im->m_priv->m_mouseOldY = 0;
    im->m_priv->m_mouseOldZ = 0;
	im->m_priv->m_newNumKeysTyped = 0;
	im->m_priv->m_mouseCallback = NULL;

    im->m_windowHasFocus = true;
    im->m_lmb = false;
    im->m_mmb = false;
    im->m_rmb = false;
	im->m_lmbDoubleClicked = false;
    im->m_lmbClicked = false;
    im->m_mmbClicked = false;
    im->m_rmbClicked = false;
    im->m_lmbUnClicked = false;
    im->m_mmbUnClicked = false;
    im->m_rmbUnClicked = false;
    im->m_mouseX = 0;
    im->m_mouseY = 0;
    im->m_mouseZ = 0;
    im->m_mouseVelX = 0;
    im->m_mouseVelY = 0;
    im->m_mouseVelZ = 0;
	im->m_numKeysTyped = 0;

    memset(im->g_keys, 0, KEY_MAX);
	memset(im->g_keyDowns, 0, KEY_MAX);
	memset(im->g_keyUps, 0, KEY_MAX);
	
    memset(im->m_priv->m_keyNewDowns, 0, KEY_MAX);
	memset(im->m_priv->m_keyNewUps, 0, KEY_MAX);

    return im;
}


// Returns 0 if the event is handled here, -1 otherwise
int EventHandler(InputManager *im, unsigned int message, unsigned int wParam, int lParam, bool /*_isAReplayedEvent*/)
{
	switch (message)
	{
		case WM_SETFOCUS:
		case WM_NCACTIVATE:
			im->m_windowHasFocus = true;
			// Clear keyboard state when we regain focus
			memset(im->m_priv->m_keyNewDowns, 0, KEY_MAX);
			memset(im->m_priv->m_keyNewUps, 0, KEY_MAX);
			memset(im->g_keys, 0, KEY_MAX);
			return -1;
		case WM_KILLFOCUS:
			im->m_windowHasFocus = false;
            im->m_mouseX = -1;
            im->m_mouseY = -1;
			break;

        case WM_SYSCHAR:
            break;

		case WM_CHAR:
			if (im->m_numKeysTyped < MAX_KEYS_TYPED_PER_FRAME &&
				!im->g_keys[KEY_CONTROL] &&
				!im->g_keys[KEY_ALT] &&
				wParam != KEY_ESC)
			{
				ReleaseAssert(wParam < KEY_MAX,
					"Keypress value out of range (WM_CHAR: wParam = %d)", wParam);
				im->m_priv->m_newKeysTyped[im->m_priv->m_newNumKeysTyped] = wParam;
				im->m_priv->m_newNumKeysTyped++;
			}
			break;

		case WM_LBUTTONDOWN:
			im->m_priv->m_lmbPrivate = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_LBUTTONUP:
			im->m_priv->m_lmbPrivate = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_MBUTTONDOWN:
			im->m_mmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_MBUTTONUP:
			im->m_mmb = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_RBUTTONDOWN:
			im->m_rmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_RBUTTONUP:
			im->m_rmb = false;
//			g_windowManager->UncaptureMouse();
			break;

		/* Mouse clicks in the Non-client area (ie titlebar) of the window are weird. If we
		   handle them and return 0, then Windows ignores them and it is no longer possible
		   to drag the window by the title bar. If we handle them and return -1, then Windows
		   never sends us the button up event. I've chosen the second option and fix some of
		   the brokenness by generating a fake up event one frame after the down event. */
		case WM_NCLBUTTONDOWN:
			im->m_priv->m_lmbPrivate = true;
			im->m_priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCMBUTTONDOWN:
			im->m_mmb = true;
			im->m_priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCRBUTTONDOWN:
			im->m_rmb = true;
			im->m_priv->m_lastClickWasNC = true;
			return -1;
			break;

		case 0x020A:
//		case WM_MOUSEWHEEL:
		{
			int move = (short)HIWORD(wParam) / 120;
			im->m_mouseZ += move;
			break;
		}

		case WM_NCMOUSEMOVE:
			im->m_mouseX = -1;
			im->m_mouseY = -1;
			break;

		case WM_MOUSEMOVE:
		{
			short newPosX = lParam & 0xFFFF;
			short newPosY = short(lParam >> 16);
			im->m_mouseX = newPosX;
			im->m_mouseY = newPosY;

			if (im->m_priv->m_mouseCallback)
			{
				im->m_priv->m_mouseCallback(newPosX, newPosY);
			}
			break;
		}

		case WM_SYSKEYUP:
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYUP: wParam = %d)", wParam);

            // If the key event is the Control part of Alt_Gr, throw it away. There
            // will be an event for the alt part too.
            if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
                break;

            im->g_keys[wParam] = 0;
			im->m_priv->m_keyNewUps[wParam] = 1;
			break;

		case WM_SYSKEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYDOWN: wParam = %d)", wParam);
			//int flags = (short)HIWORD(lParam);
			im->g_keys[wParam] = 1;
			im->m_priv->m_keyNewDowns[wParam] = 1;
			break;
		}

		case WM_KEYUP:
		{
			// Alt key ups are presented here when the user keys, for example, Alt+F.
			// Windows will generate a SYSKEYUP event for the release of F, and a
			// normal KEYUP event for the release of the ALT. Very strange.
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYUP: wParam = %d)", wParam);
			im->g_keys[wParam] = 0;
			im->m_priv->m_keyNewUps[wParam] = 1;
			break;
		}

		case WM_KEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYDOWN: wParam = %d)", wParam);
			if (wParam == KEY_DEL)
			{
				im->m_priv->m_newKeysTyped[im->m_numKeysTyped] = 127;
				im->m_priv->m_newNumKeysTyped++;
			}
            else if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
            {
                //int res = GetAsyncKeyState(VK_MENU);
                // Key event is the Control part of Alt_Gr, so throw it away. There
                // will be an event for the alt part too.
                break;
            }
			im->m_priv->m_keyNewDowns[wParam] = 1;
			im->g_keys[wParam] = 1;
			break;
		}

		default:
			return -1;
	}

	return 0;
}


void InputManagerAdvance(InputManager *im)
{
	memcpy(im->g_keyDowns, im->m_priv->m_keyNewDowns, KEY_MAX);
	memcpy(im->g_keyUps, im->m_priv->m_keyNewUps, KEY_MAX);
	memset(im->m_priv->m_keyNewDowns, 0, KEY_MAX);
	memset(im->m_priv->m_keyNewUps, 0, KEY_MAX);

	im->m_numKeysTyped = im->m_priv->m_newNumKeysTyped;
	memcpy(im->m_keysTyped, im->m_priv->m_newKeysTyped, im->m_priv->m_newNumKeysTyped);
	im->m_priv->m_newNumKeysTyped = 0;


	// Count the number of key ups and downs this frame
	im->m_numKeyDowns = 0;
	im->m_numKeyUps = 0;
	for (int i = 0; i < KEY_MAX; ++i)
	{
		if (im->g_keyUps[i]) im->m_numKeyUps++;
		if (im->g_keyDowns[i]) im->m_numKeyDowns++;
	}

    im->m_lmb = im->m_priv->m_lmbPrivate;
	im->m_lmbClicked = im->m_lmb == true && im->m_priv->m_lmbOld == false;
	im->m_mmbClicked = im->m_mmb == true && im->m_priv->m_mmbOld == false;
	im->m_rmbClicked = im->m_rmb == true && im->m_priv->m_rmbOld == false;
	im->m_priv->m_rawLmbClicked = im->m_lmbClicked;
	im->m_lmbUnClicked = im->m_lmb == false && im->m_priv->m_lmbOld == true;
	im->m_mmbUnClicked = im->m_mmb == false && im->m_priv->m_mmbOld == true;
	im->m_rmbUnClicked = im->m_rmb == false && im->m_priv->m_rmbOld == true;
	im->m_priv->m_lmbOld = im->m_lmb;
	im->m_priv->m_mmbOld = im->m_mmb;
	im->m_priv->m_rmbOld = im->m_rmb;

	im->m_lmbDoubleClicked = false;
	if (im->m_lmbClicked)
	{
 		double timeNow = GetHighResTime();
		double delta = timeNow - im->m_priv->m_lastClickTime;
		if (delta < 0.25)
		{
			im->m_lmbDoubleClicked = true;
			im->m_lmbClicked = false;
		}
		im->m_priv->m_lastClickTime = timeNow;
		im->m_priv->m_lmbRepeatsSoFar = 0;
	}

	// Generate repeats when the lmb is held
	im->m_lmbRepeated = false;
	if (im->m_lmb)
	{
		float timeSinceClick = (GetHighResTime() - im->m_priv->m_lastClickTime);
		int maxPossibleRepeats = (int)(timeSinceClick / LMB_REPEAT_PERIOD);
		if (maxPossibleRepeats > 3 && maxPossibleRepeats > im->m_priv->m_lmbRepeatsSoFar)
		{
			im->m_priv->m_lmbRepeatsSoFar = maxPossibleRepeats;
			im->m_lmbRepeated = true;
		}
	}

	// Generate fake mouse up event(s) to compensate for the fact that Windows won't send
	// them if the last mouse down event was in the client area.
	if (im->m_priv->m_lastClickWasNC)
	{
		im->m_priv->m_lastClickWasNC = false;
		im->m_priv->m_lmbPrivate = false;
		im->m_mmb = false;
		im->m_rmb = false;
	}

	im->m_mouseVelX = im->m_mouseX - im->m_priv->m_mouseOldX;
	im->m_mouseVelY = im->m_mouseY - im->m_priv->m_mouseOldY;
	im->m_mouseVelZ = im->m_mouseZ - im->m_priv->m_mouseOldZ;
	im->m_priv->m_mouseOldX = im->m_mouseX;
	im->m_priv->m_mouseOldY = im->m_mouseY;
	im->m_priv->m_mouseOldZ = im->m_mouseZ;

    MSG msg;
    while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
    {
        // handle or dispatch messages
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    } 
}


bool GetRawLmbClicked(InputManager *im) 
{ 
    return im->m_priv->m_rawLmbClicked; 
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
	{
		return atoi(name + 7);
	}

	return -1;
}


bool UntypeKey(InputManager *im, char key)
{
	for (int i = 0; i < im->m_numKeysTyped; ++i)
	{
		if (im->m_keysTyped[i] == key)
		{
			im->m_keysTyped[i] = im->m_keysTyped[im->m_numKeysTyped - 1];
			im->m_numKeysTyped--;
			return true;
		}
	}

	return false;
}


void RegisterMouseUpdateCallback(InputManager *im, MouseUpdateCallback *func) 
{ 
    im->m_priv->m_mouseCallback = func; 
}

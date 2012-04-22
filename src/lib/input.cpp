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
    im->priv = new InputManagerPrivate;

    im->priv->m_lmbPrivate = false;
    im->priv->m_lmbOld = false;
    im->priv->m_mmbOld = false;
    im->priv->m_rmbOld = false;
	im->priv->m_lastClickWasNC = false;
	im->priv->m_lastClickTime = 0.0;
	im->priv->m_lmbRepeatsSoFar = 0;
	im->priv->m_rawLmbClicked = false;
    im->priv->m_mouseOldX = 0;
    im->priv->m_mouseOldY = 0;
    im->priv->m_mouseOldZ = 0;
	im->priv->m_newNumKeysTyped = 0;
	im->priv->m_mouseCallback = NULL;

    im->windowHasFocus = true;
    im->lmb = false;
    im->mmb = false;
    im->rmb = false;
	im->lmbDoubleClicked = false;
    im->lmbClicked = false;
    im->mmbClicked = false;
    im->rmbClicked = false;
    im->lmbUnClicked = false;
    im->mmbUnClicked = false;
    im->rmbUnClicked = false;
    im->mouseX = 0;
    im->mouseY = 0;
    im->mouseZ = 0;
    im->mouseVelX = 0;
    im->mouseVelY = 0;
    im->mouseVelZ = 0;
	im->numKeysTyped = 0;

    memset(im->keys, 0, KEY_MAX);
	memset(im->keyDowns, 0, KEY_MAX);
	memset(im->keyUps, 0, KEY_MAX);
	
    memset(im->priv->m_keyNewDowns, 0, KEY_MAX);
	memset(im->priv->m_keyNewUps, 0, KEY_MAX);

    return im;
}


// Returns 0 if the event is handled here, -1 otherwise
int EventHandler(InputManager *im, unsigned int message, unsigned int wParam, int lParam, bool /*_isAReplayedEvent*/)
{
	switch (message)
	{
		case WM_SETFOCUS:
		case WM_NCACTIVATE:
			im->windowHasFocus = true;
			// Clear keyboard state when we regain focus
			memset(im->priv->m_keyNewDowns, 0, KEY_MAX);
			memset(im->priv->m_keyNewUps, 0, KEY_MAX);
			memset(im->keys, 0, KEY_MAX);
			return -1;
		case WM_KILLFOCUS:
			im->windowHasFocus = false;
            im->mouseX = -1;
            im->mouseY = -1;
			break;

        case WM_SYSCHAR:
            break;

		case WM_CHAR:
			if (im->numKeysTyped < MAX_KEYS_TYPED_PER_FRAME &&
				!im->keys[KEY_CONTROL] &&
				!im->keys[KEY_ALT] &&
				wParam != KEY_ESC)
			{
				ReleaseAssert(wParam < KEY_MAX,
					"Keypress value out of range (WM_CHAR: wParam = %d)", wParam);
				im->priv->m_newKeysTyped[im->priv->m_newNumKeysTyped] = wParam;
				im->priv->m_newNumKeysTyped++;
			}
			break;

		case WM_LBUTTONDOWN:
			im->priv->m_lmbPrivate = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_LBUTTONUP:
			im->priv->m_lmbPrivate = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_MBUTTONDOWN:
			im->mmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_MBUTTONUP:
			im->mmb = false;
//			g_windowManager->UncaptureMouse();
			break;
		case WM_RBUTTONDOWN:
			im->rmb = true;
//			g_windowManager->CaptureMouse();
			break;
		case WM_RBUTTONUP:
			im->rmb = false;
//			g_windowManager->UncaptureMouse();
			break;

		/* Mouse clicks in the Non-client area (ie titlebar) of the window are weird. If we
		   handle them and return 0, then Windows ignores them and it is no longer possible
		   to drag the window by the title bar. If we handle them and return -1, then Windows
		   never sends us the button up event. I've chosen the second option and fix some of
		   the brokenness by generating a fake up event one frame after the down event. */
		case WM_NCLBUTTONDOWN:
			im->priv->m_lmbPrivate = true;
			im->priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCMBUTTONDOWN:
			im->mmb = true;
			im->priv->m_lastClickWasNC = true;
			return -1;
			break;
		case WM_NCRBUTTONDOWN:
			im->rmb = true;
			im->priv->m_lastClickWasNC = true;
			return -1;
			break;

		case 0x020A:
//		case WM_MOUSEWHEEL:
		{
			int move = (short)HIWORD(wParam) / 120;
			im->mouseZ += move;
			break;
		}

		case WM_NCMOUSEMOVE:
			im->mouseX = -1;
			im->mouseY = -1;
			break;

		case WM_MOUSEMOVE:
		{
			short newPosX = lParam & 0xFFFF;
			short newPosY = short(lParam >> 16);
			im->mouseX = newPosX;
			im->mouseY = newPosY;

			if (im->priv->m_mouseCallback)
			{
				im->priv->m_mouseCallback(newPosX, newPosY);
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

            im->keys[wParam] = 0;
			im->priv->m_keyNewUps[wParam] = 1;
			break;

		case WM_SYSKEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_SYSKEYDOWN: wParam = %d)", wParam);
			//int flags = (short)HIWORD(lParam);
			im->keys[wParam] = 1;
			im->priv->m_keyNewDowns[wParam] = 1;
			break;
		}

		case WM_KEYUP:
		{
			// Alt key ups are presented here when the user keys, for example, Alt+F.
			// Windows will generate a SYSKEYUP event for the release of F, and a
			// normal KEYUP event for the release of the ALT. Very strange.
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYUP: wParam = %d)", wParam);
			im->keys[wParam] = 0;
			im->priv->m_keyNewUps[wParam] = 1;
			break;
		}

		case WM_KEYDOWN:
		{
			ReleaseAssert(wParam < KEY_MAX,
				"Keypress value out of range (WM_KEYDOWN: wParam = %d)", wParam);
			if (wParam == KEY_DEL)
			{
				im->priv->m_newKeysTyped[im->numKeysTyped] = 127;
				im->priv->m_newNumKeysTyped++;
			}
            else if (wParam == KEY_CONTROL && GetAsyncKeyState(VK_MENU) < 0)
            {
                //int res = GetAsyncKeyState(VK_MENU);
                // Key event is the Control part of Alt_Gr, so throw it away. There
                // will be an event for the alt part too.
                break;
            }
			im->priv->m_keyNewDowns[wParam] = 1;
			im->keys[wParam] = 1;
			break;
		}

		default:
			return -1;
	}

	return 0;
}


void InputManagerAdvance(InputManager *im)
{
	memcpy(im->keyDowns, im->priv->m_keyNewDowns, KEY_MAX);
	memcpy(im->keyUps, im->priv->m_keyNewUps, KEY_MAX);
	memset(im->priv->m_keyNewDowns, 0, KEY_MAX);
	memset(im->priv->m_keyNewUps, 0, KEY_MAX);

	im->numKeysTyped = im->priv->m_newNumKeysTyped;
	memcpy(im->keysTyped, im->priv->m_newKeysTyped, im->priv->m_newNumKeysTyped);
	im->priv->m_newNumKeysTyped = 0;


	// Count the number of key ups and downs this frame
	im->numKeyDowns = 0;
	im->numKeyUps = 0;
	for (int i = 0; i < KEY_MAX; ++i)
	{
		if (im->keyUps[i]) im->numKeyUps++;
		if (im->keyDowns[i]) im->numKeyDowns++;
	}

    im->lmb = im->priv->m_lmbPrivate;
	im->lmbClicked = im->lmb == true && im->priv->m_lmbOld == false;
	im->mmbClicked = im->mmb == true && im->priv->m_mmbOld == false;
	im->rmbClicked = im->rmb == true && im->priv->m_rmbOld == false;
	im->priv->m_rawLmbClicked = im->lmbClicked;
	im->lmbUnClicked = im->lmb == false && im->priv->m_lmbOld == true;
	im->mmbUnClicked = im->mmb == false && im->priv->m_mmbOld == true;
	im->rmbUnClicked = im->rmb == false && im->priv->m_rmbOld == true;
	im->priv->m_lmbOld = im->lmb;
	im->priv->m_mmbOld = im->mmb;
	im->priv->m_rmbOld = im->rmb;

	im->lmbDoubleClicked = false;
	if (im->lmbClicked)
	{
 		double timeNow = GetHighResTime();
		double delta = timeNow - im->priv->m_lastClickTime;
		if (delta < 0.25)
		{
			im->lmbDoubleClicked = true;
			im->lmbClicked = false;
		}
		im->priv->m_lastClickTime = timeNow;
		im->priv->m_lmbRepeatsSoFar = 0;
	}

	// Generate repeats when the lmb is held
	im->lmbRepeated = false;
	if (im->lmb)
	{
		float timeSinceClick = (GetHighResTime() - im->priv->m_lastClickTime);
		int maxPossibleRepeats = (int)(timeSinceClick / LMB_REPEAT_PERIOD);
		if (maxPossibleRepeats > 3 && maxPossibleRepeats > im->priv->m_lmbRepeatsSoFar)
		{
			im->priv->m_lmbRepeatsSoFar = maxPossibleRepeats;
			im->lmbRepeated = true;
		}
	}

	// Generate fake mouse up event(s) to compensate for the fact that Windows won't send
	// them if the last mouse down event was in the client area.
	if (im->priv->m_lastClickWasNC)
	{
		im->priv->m_lastClickWasNC = false;
		im->priv->m_lmbPrivate = false;
		im->mmb = false;
		im->rmb = false;
	}

	im->mouseVelX = im->mouseX - im->priv->m_mouseOldX;
	im->mouseVelY = im->mouseY - im->priv->m_mouseOldY;
	im->mouseVelZ = im->mouseZ - im->priv->m_mouseOldZ;
	im->priv->m_mouseOldX = im->mouseX;
	im->priv->m_mouseOldY = im->mouseY;
	im->priv->m_mouseOldZ = im->mouseZ;

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
    return im->priv->m_rawLmbClicked; 
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
	for (int i = 0; i < im->numKeysTyped; ++i)
	{
		if (im->keysTyped[i] == key)
		{
			im->keysTyped[i] = im->keysTyped[im->numKeysTyped - 1];
			im->numKeysTyped--;
			return true;
		}
	}

	return false;
}


void RegisterMouseUpdateCallback(InputManager *im, MouseUpdateCallback *func) 
{ 
    im->priv->m_mouseCallback = func; 
}

#ifndef INCLUDED_INPUT_H
#define INCLUDED_INPUT_H


#include "lib/common.h"


#define KEY_MAX						256
#define MAX_KEYS_TYPED_PER_FRAME	128


typedef void MouseUpdateCallback(int x, int y);


struct InputManagerPrivate;


struct InputManager
{
    InputManagerPrivate *m_priv;

    bool		m_windowHasFocus;

    bool		m_lmb;					// Mouse button states now. These can change in the 
	bool		m_mmb;					// middle of the frame.
    bool		m_rmb;

	bool		m_lmbDoubleClicked;
	bool		m_lmbRepeated;			// Becomes true for the duration of one advance once per LMB_REPEAT_PERIOD

    bool		m_lmbClicked;			// Mouse went from up to down this frame
	bool		m_mmbClicked;			// These can be modified by the application, to prevent
    bool		m_rmbClicked;			// mouse clicks occurring when you don't want them to
										// See m_rawLmbClicked etc for none frigged data

    bool		m_lmbUnClicked;			// Mouse went from down to up this frame
	bool		m_mmbUnClicked;
    bool		m_rmbUnClicked;

    int			m_mouseX;				// Mouse pos now. Can changes through the course of the frame
    int			m_mouseY;
    int			m_mouseZ;

    int			m_mouseVelX;
    int			m_mouseVelY;
    int			m_mouseVelZ;

	char		m_keysTyped[MAX_KEYS_TYPED_PER_FRAME];
	int			m_numKeysTyped;

	int			m_numKeyUps;
	int			m_numKeyDowns;

    signed char g_keys[KEY_MAX];		// Is the key currently pressed
    signed char g_keyDowns[KEY_MAX];	// Was the key pressed this frame (includes key repeats)
    signed char g_keyUps[KEY_MAX];	    // Was the key released this frame
};


InputManager	*CreateInputManager();
    
void	        InputManagerAdvance(InputManager *im);
int 	        EventHandler(InputManager *im, unsigned int _eventId, unsigned int wParam, int lParam, bool _isAReplayedEvent = false);

bool	        GetRawLmbClicked(InputManager *im);
char const *    GetKeyName(int i);
int		        GetKeyId(char const *name);
bool	        UntypeKey(InputManager *im, char key);	// Removes the specified key from the m_keysTypes array. Returns false if key was not present

void	        RegisterMouseUpdateCallback(InputManager *im, MouseUpdateCallback *func);


#define KEY_A                 65
#define KEY_B                 66
#define KEY_C                 67
#define KEY_D                 68
#define KEY_E                 69
#define KEY_F                 70
#define KEY_G                 71
#define KEY_H                 72
#define KEY_I                 73
#define KEY_J                 74
#define KEY_K                 75
#define KEY_L                 76
#define KEY_M                 77
#define KEY_N                 78
#define KEY_O                 79
#define KEY_P                 80
#define KEY_Q                 81
#define KEY_R                 82
#define KEY_S                 83
#define KEY_T                 84
#define KEY_U                 85
#define KEY_V                 86
#define KEY_W                 87
#define KEY_X                 88
#define KEY_Y                 89
#define KEY_Z                 90
#define KEY_0                 48
#define KEY_1                 49
#define KEY_2                 50
#define KEY_3                 51
#define KEY_4                 52
#define KEY_5                 53
#define KEY_6                 54
#define KEY_7                 55
#define KEY_8                 56
#define KEY_9                 57
#define KEY_MENU              93
#define KEY_0_PAD             96
#define KEY_1_PAD             97
#define KEY_2_PAD             98
#define KEY_3_PAD             99
#define KEY_4_PAD             100
#define KEY_5_PAD             101
#define KEY_6_PAD             102
#define KEY_7_PAD             103
#define KEY_8_PAD             104
#define KEY_9_PAD             105
#define KEY_F1                112
#define KEY_F2                113
#define KEY_F3                114
#define KEY_F4                115
#define KEY_F5                116
#define KEY_F6                117
#define KEY_F7                118
#define KEY_F8                119
#define KEY_F9                120
#define KEY_F10               121
#define KEY_F11               122
#define KEY_F12               123
#define KEY_ESC               27
#define KEY_TILDE             223
#define KEY_MINUS             189
#define KEY_EQUALS            187
#define KEY_BACKSPACE         8
#define KEY_TAB               9
#define KEY_OPENBRACE         219
#define KEY_CLOSEBRACE        221
#define KEY_ENTER             13
#define KEY_COLON             186
#define KEY_QUOTE             192
#define KEY_BACKSLASH         220
#define KEY_COMMA             188
#define KEY_STOP              190
#define KEY_SLASH             191
#define KEY_SPACE             32
#define KEY_INSERT            45
#define KEY_DEL               46
#define KEY_HOME              36
#define KEY_END               35
#define KEY_PGUP              33
#define KEY_PGDN              34
#define KEY_LEFT              37
#define KEY_RIGHT             39
#define KEY_UP                38
#define KEY_DOWN              40
#define KEY_SLASH_PAD         111
#define KEY_ASTERISK          106
#define KEY_MINUS_PAD         109
#define KEY_PLUS_PAD          107
#define KEY_DEL_PAD           110
#define KEY_PAUSE             19
#define KEY_AT                192
#define KEY_ALT               18
#define KEY_SCRLOCK           145
#define KEY_NUMLOCK           144  
#define KEY_CAPSLOCK          20
#define KEY_SHIFT			  16
#define KEY_CONTROL			  17

#endif

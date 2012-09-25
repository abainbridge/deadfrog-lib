from ctypes import *

dll = CDLL("deadfrog-lib.dll")

def makenot(name, argtypes, restype):
    rv = eval("dll." + name)
    rv.argtypes = argtypes
    rv.restype = restype
    return rv

def makevoid(name, argtypes):
    return makenot(name, argtypes, None)


# *****************************************************************************
# InputManager
# *****************************************************************************

class InputManager(Structure):
    _fields_ = [("private_data", c_void_p),

                ("windowHasFocus", c_bool),
   
                ("lmb", c_bool),
                ("mmb", c_bool),
                ("rmb", c_bool),
   
                ("lmbDoubleClicked", c_bool),
                ("lmbRepeated", c_bool),
   
                ("lmbClicked", c_bool),
                ("mmbClicked", c_bool),
                ("rmbClicked", c_bool),
      
                ("lmbUnClicked", c_bool),
                ("mmbUnClicked", c_bool),
                ("rmbUnClicked", c_bool),
   
                ("mouseX", c_int),
                ("mouseY", c_int),
                ("mouseZ", c_int),
   
                ("mouseVelX", c_int),
                ("mouseVelY", c_int),
                ("mouseVelZ", c_int),
   
                ("keysTyped", c_char * 128),
                ("numKeysTyped", c_int),
   
                ("numKeyUps", c_int),
                ("numKeyDowns", c_int),
                
                ("keys", c_byte * 256),
                ("keyDowns", c_char * 256),
                ("keyUps", c_char * 256)]

# Define constants that can be used as indices into "keys", "keyDowns" and "keyUps"
# ord('A') to ord('Z') are also acceptable
KEY_MENU              = 93
KEY_0_PAD             = 96
KEY_1_PAD             = 97
KEY_2_PAD             = 98
KEY_3_PAD             = 99
KEY_4_PAD             = 100
KEY_5_PAD             = 101
KEY_6_PAD             = 102
KEY_7_PAD             = 103
KEY_8_PAD             = 104
KEY_9_PAD             = 105
KEY_F1                = 112
KEY_F2                = 113
KEY_F3                = 114
KEY_F4                = 115
KEY_F5                = 116
KEY_F6                = 117
KEY_F7                = 118
KEY_F8                = 119
KEY_F9                = 120
KEY_F10               = 121
KEY_F11               = 122
KEY_F12               = 123
KEY_ESC               = 27
KEY_TILDE             = 223
KEY_MINUS             = 189
KEY_EQUALS            = 187
KEY_BACKSPACE         = 8
KEY_TAB               = 9
KEY_OPENBRACE         = 219
KEY_CLOSEBRACE        = 221
KEY_ENTER             = 13
KEY_COLON             = 186
KEY_QUOTE             = 192
KEY_BACKSLASH         = 220
KEY_COMMA             = 188
KEY_STOP              = 190
KEY_SLASH             = 191
KEY_SPACE             = 32
KEY_INSERT            = 45
KEY_DEL               = 46
KEY_HOME              = 36
KEY_END               = 35
KEY_PGUP              = 33
KEY_PGDN              = 34
KEY_LEFT              = 37
KEY_RIGHT             = 39
KEY_UP                = 38
KEY_DOWN              = 40
KEY_SLASH_PAD         = 111
KEY_ASTERISK          = 106
KEY_MINUS_PAD         = 109
KEY_PLUS_PAD          = 107
KEY_DEL_PAD           = 110
KEY_PAUSE             = 19
KEY_AT                = 192
KEY_ALT               = 18
KEY_SCRLOCK           = 145
KEY_NUMLOCK           = 144  
KEY_CAPSLOCK          = 20
KEY_SHIFT			  = 16
KEY_CONTROL			  = 17

                
# *****************************************************************************
# Window
# *****************************************************************************

class WindowData(Structure):
    _fields_ = [("private_data", c_void_p),
                ("bmp", c_void_p),
                ("width", c_uint),
                ("height", c_uint),
                ("inputManager", POINTER(InputManager)),
                ("windowClosed", c_bool),
                ("fps", c_uint)]

CreateWin = makenot("CreateWin", [c_int, c_int, c_int, c_int, c_bool, c_char_p], POINTER(WindowData))
AdvanceWin = makenot("AdvanceWin", [POINTER(WindowData)], c_void_p)
BlitBitmap = makevoid("BlitBitmap", [POINTER(WindowData), c_void_p, c_int, c_int])


# *****************************************************************************
# Bitmap
# *****************************************************************************

CreateBitmapRGBA = makenot("CreateBitmapRGBA", [c_uint, c_uint], c_void_p)
ClearBitmap = makevoid("ClearBitmap", [c_void_p, c_uint])
Plot = makevoid("Plot", [c_void_p, c_uint, c_uint, c_uint])
DrawLine = makevoid("DrawLine", [c_void_p, c_int, c_int, c_int, c_int, c_uint])
RectFill = makevoid("RectFill", [c_void_p, c_int, c_int, c_uint, c_uint, c_uint])


# *****************************************************************************
# TextRenderer
# *****************************************************************************

CreateTextRenderer = makenot("CreateTextRenderer", [c_char_p, c_int, c_bool], c_void_p)
DrawTextSimple = makenot("DrawTextSimple", 
                         [c_void_p, c_uint, c_void_p, c_uint, c_uint, c_char_p], c_int)
DrawTextCentre = makenot("DrawTextCentre", 
                         [c_void_p, c_uint, c_void_p, c_uint, c_uint, c_char_p], c_int)


# *****************************************************************************
# Graph3d
# *****************************************************************************

CreateGraph3d = makenot("CreateGraph3d", None, c_void_p)
Graph3dAddPoint = makevoid("Graph3dAddPoint", [c_void_p, c_float, c_float, c_float, c_uint])
Graph3dRender = makevoid("Graph3dRender", 
                         [c_void_p, c_void_p, c_float, c_float, c_float, c_float, c_uint, 
                          c_float, c_float])
   
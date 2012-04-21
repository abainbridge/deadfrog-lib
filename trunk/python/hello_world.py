from deadfroglib import *

# set up the window
screen_w = 800
screen_h = 600
win = CreateWin(50, 50, screen_w, screen_h, True, 'Hello World Example')

# Choose a font
font = CreateTextRenderer("Courier", 8, True)
 
# set up the colors
BLACK = 0x00000000
WHITE = 0xffffffff

while not win.contents.windowClosed:
    bmp = AdvanceWin(win)
    ClearBitmap(bmp, WHITE)
    DrawTextCentre(font, BLACK, bmp, screen_w/2, screen_h/2, "Hello World!")

import bainlib

# set up the window
screen_w = 800
screen_h = 600
win = bainlib.CreateWin(50, 50, screen_w, screen_h, True, 'Hello World Example')

# Choose a font
font = bainlib.CreateTextRenderer("Courier", 8, True)
 
# set up the colors
BLACK = 0x00000000
WHITE = 0xffffffff

while not win.contents.windowClosed:
    bmp = bainlib.AdvanceWin(win)
    bainlib.ClearBitmap(bmp, WHITE)
    bainlib.DrawTextCentre(font, BLACK, bmp, screen_w/2, screen_h/2, "Hello World!")

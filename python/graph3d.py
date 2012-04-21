from bainlib import *
import Image

im = Image.open("small.png")
graph3d = CreateGraph3d()
for y in range(im.size[1]):
    for x in range(im.size[0]):
        p = im.getpixel((x, y))
        (r, g, b) = p
        ya = 0.299*r + 0.587*g + 0.114*b
        cb = 128 - 0.168736*r - 0.331264*g + 0.5*b
        cr = 128 + 0.5*r - 0.418688*g - 0.081312*b
        Graph3dAddPoint(graph3d, ya-128, cr-128, cb-128)
Graph3dAddPoint(graph3d, -128, -128, -128)
Graph3dAddPoint(graph3d, 128, 128, 128)

# set up the window
screenw = 1000
screenh = 1000
win = CreateWin(500, 50, screenw, screenh, True, '3d plot')
input = win.contents.inputManager.contents
font = CreateTextRenderer("Fixedsys", 8, True)
 
# set up the colors
BLACK = 0xff000000
WHITE = 0xffffffff

dist = 430.0
zoom = 1200.0
rotZ = 0.0

while not win.contents.windowClosed and input.keys[KEY_ESC] == 0:
    bmp = AdvanceWin(win)

    ClearBitmap(bmp, WHITE)

    if input.lmb:
        rotZ += float(input.mouseVelX) * 0.01

    zoom *= 1.0 + (input.mouseVelZ * 0.05)

    Graph3dRender(graph3d, bmp, screenw / 2, screenh / 2, dist, zoom, BLACK, rotZ)

    DrawTextSimple(font, BLACK, bmp, screenw - 100, 5, str(win.contents.fps))
    
    RectFill(bmp, 0, screenh - 40, 230, 40, WHITE)
    DrawTextSimple(font, BLACK, bmp, 10, screenh - 35, "Hold left mouse to rotate")
    DrawTextSimple(font, BLACK, bmp, 10, screenh - 20, "Mouse wheel to zoom")

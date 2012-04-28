from deadfroglib import *
import Image
import math

# set up the colors
BLACK = 0xff000000
WHITE = 0xffffffff

im = Image.open("willow.bmp")
graph3d = CreateGraph3d()
err = 0.0
for y in range(im.size[1]):
    for x in range(im.size[0]):
        (r, g, b) = im.getpixel((x, y))
        
        ya = round(0.299*r + 0.587*g + 0.114*b)
        cb = round(128 - 0.168736*r - 0.331264*g + 0.5*b)
        cr = round(128 + 0.5*r - 0.418688*g - 0.081312*b)
        
#         r2 = ya + 1.402 * (cr - 128)
#         g2 = ya - 0.34414 * (cb - 128) - 0.71414 * (cr - 128)
#         b2 = ya + 1.772 * (cb - 128)
# #         if r != r2 or g != g2 or b != b2:
# #         print r,g,b
# #         print r2,g2,b2
# #         print
#         err += math.sqrt((r - round(r2)) ** 2 +
#                         (g - round(g2)) ** 2 +
#                         (b - round(b2)) ** 2)
        
        col = (r << 16) + (g << 8) + b
        Graph3dAddPoint(graph3d, ya-128, cr-128, cb-128, col)
#        Graph3dAddPoint(graph3d, r-128, g-128, b-128, col)
Graph3dAddPoint(graph3d, -128, -128, -128, WHITE)
Graph3dAddPoint(graph3d, 128, 128, 128, WHITE)
print "err", err

# set up the window
screenw = 600
screenh = 600
win = CreateWin(500, 50, screenw, screenh, True, '3d plot')
input = win.contents.inputManager.contents
font = CreateTextRenderer("Fixedsys", 8, True)
 
dist = 430.0
zoom = 700.0
rotX = 0.0
rotZ = 0.0

while not win.contents.windowClosed and input.keys[KEY_ESC] == 0:
    bmp = AdvanceWin(win)

    ClearBitmap(bmp, WHITE)

    if input.lmb:
        rotX -= float(input.mouseVelY) * 0.01
        rotZ += float(input.mouseVelX) * 0.01
    #rotZ += 0.03

    zoom *= 1.0 + (input.mouseVelZ * 0.05)

    Graph3dRender(graph3d, bmp, screenw / 2, screenh / 2, dist, zoom, BLACK, rotX, rotZ)

    DrawTextSimple(font, BLACK, bmp, screenw - 100, 5, str(win.contents.fps))
    
    RectFill(bmp, 0, screenh - 40, 230, 40, WHITE)
    DrawTextSimple(font, BLACK, bmp, 10, screenh - 35, "Hold left mouse to rotate")
    DrawTextSimple(font, BLACK, bmp, 10, screenh - 20, "Mouse wheel to zoom")

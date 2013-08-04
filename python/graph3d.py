from deadfroglib import *
import Image
import math

# set up the colors
BLACK = 0xff000000
WHITE = 0xffffffff

im = Image.open("willow.bmp")
imOut = Image.new(im.mode, im.size)
graph3d = CreateGraph3d()
minA = 1000
maxA = -1000
minB = 1000
maxB = -1000
minC = 1000
maxC = -1000
err = 0.0
for y in range(im.size[1]):
    for x in range(im.size[0]):
        (r, g, b) = im.getpixel((x, y))
        
#         ya = round(0.299*r + 0.587*g + 0.114*b)
#         cb = round(128 - 0.168736*r - 0.331264*g + 0.5*b)
#         cr = round(128 + 0.5*r - 0.418688*g - 0.081312*b)

#         ya = round( r + g + b)
#         cb = round( r - g)
#         cr = round( r + g - 2*b)

        ya = round((+0.61333*r + 0.58095*g + 0.53509*b) * 0.575)
        cb = round((-0.32762*r + 0.80357*g - 0.49693*b) * 0.575) + 128
        cr = round((+0.71868*r - 0.12948*g - 0.68318*b) * 0.575) + 128
#        {{0.61333, 0.58095, 0.53509}, {-0.32762, 0.80357, -0.49693}, {0.71868, -0.12948, -0.68318}}

        if ya < minA: minA = ya
        if ya > maxA: maxA = ya
        if cr < minB: minB = cr
        if cr > maxB: maxB = cr
        if cb < minC: minC = cb
        if cb > maxC: maxC = cb

        # Invert Y Cr Cb
        r2 = ya + 1.402 * (cr - 128)
        g2 = ya - 0.34414 * (cb - 128) - 0.71414 * (cr - 128)
        b2 = ya + 1.772 * (cb - 128)

#         # Invert custom colour space
#         cr -= 128
#         cb -= 128
#         ya /= 0.575
#         cr /= 0.575
#         cb /= 0.575
#         r2 = 0.61333 * ya - 0.32762 * cb + 0.71868 * cr
#         g2 = 0.58095 * ya + 0.80357 * cb - 0.12948 * cr
#         b2 = 0.53509 * ya - 0.49693 * cb - 0.68318 * cr

#         if r != r2 or g != g2 or b != b2:
#         print "%5.2f %5.2f %5.2f" % (r,g,b)
#         print "%5.2f %5.2f %5.2f" % (r2,g2,b2)
#         print

        # Calc RMS error for this pixel
        err += math.sqrt((r - round(r2)) ** 2 +
                        (g - round(g2)) ** 2 +
                        (b - round(b2)) ** 2)

        col = (r << 16) + (g << 8) + b
#        Graph3dAddPoint(graph3d, ya-128, cr, cb, col)
#        print "%6.2f %6.2f %6.2f" % (ya, cr, cb)
        Graph3dAddPoint(graph3d, r-128, g-128, b-128, col)
    
        imOut.putpixel((x,y), (int(ya), int(cr), int(cb)))

imOut.save("foo.bmp")
print "Min:", minA, minB, minC
print "Max:", maxA, maxB, maxC
#Graph3dAddPoint(graph3d, -128, -128, -128, WHITE)
#Graph3dAddPoint(graph3d, 128, 128, 128, WHITE)
print "err", err / (im.size[0] * im.size[1])

# set up the window
screenw = 600
screenh = 600
win = CreateWin(500, 50, screenw, screenh, True, '3d plot')
input = win.contents.inputManager.contents
font = CreateTextRenderer("Fixedsys", 8, True)
 
dist = 730.0
zoom = 800.0
rotX = 0.0
rotZ = 0.0
cx = screenw / 2
cy = screenh / 2

while not win.contents.windowClosed and input.keys[KEY_ESC] == 0:
    bmp = AdvanceWin(win)

    ClearBitmap(bmp, WHITE)

    if input.lmb:
        if input.keys[KEY_SHIFT]:
            cx += input.mouseVelX
            cy += input.mouseVelY
        else:
            rotX -= float(input.mouseVelY) * 0.01
            rotZ += float(input.mouseVelX) * 0.01
    #rotZ += 0.03

    zoom *= 1.0 + (input.mouseVelZ * 0.05)

    Graph3dRender(graph3d, bmp, cx, cy, dist, zoom, BLACK, rotX, rotZ)

    DrawTextSimple(font, BLACK, bmp, screenw - 100, 5, str(win.contents.fps))
    
    RectFill(bmp, 0, screenh - 40, 230, 40, WHITE)
    DrawTextSimple(font, BLACK, bmp, 10, screenh - 35, "Hold left mouse to rotate")
    DrawTextSimple(font, BLACK, bmp, 10, screenh - 20, "Mouse wheel to zoom")

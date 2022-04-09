#include "df_colour.h"


DfColour g_colourBlack = { 0xff000000U };
DfColour g_colourWhite = { 0xffffffffU };


DfColour RgbaAddWithSaturate(DfColour a, DfColour b) {
    int x = a.r + b.r;
    int y = a.g + b.g;
    int z = a.b + b.b;
    int w = a.a + b.a;

    x = ClampInt(x, 0, 255);
    y = ClampInt(y, 0, 255);
    z = ClampInt(z, 0, 255);
    w = ClampInt(w, 0, 255);

    return Colour(x, y, z, w);
}

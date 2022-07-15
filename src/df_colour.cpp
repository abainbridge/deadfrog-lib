#include "df_colour.h"


DfColour g_colourBlack = { 0xff000000U };
DfColour g_colourWhite = { 0xffffffffU };


DfColour RgbaAddWithSaturate(DfColour _a, int r, int g, int b) {
    int x = _a.r + r;
    int y = _a.g + g;
    int z = _a.b + b;

    x = ClampInt(x, 0, 255);
    y = ClampInt(y, 0, 255);
    z = ClampInt(z, 0, 255);

    return Colour(x, y, z);
}


DfColour RgbaBlendTowards(DfColour a, DfColour b, float fraction) {
    float inverseFraction = 1.0f - fraction;
    DfColour rv;
    rv.r = (float)b.r * fraction + (float)a.r * inverseFraction;
    rv.g = (float)b.g * fraction + (float)a.g * inverseFraction;
    rv.b = (float)b.b * fraction + (float)a.b * inverseFraction;
    rv.a = 255;
    return rv;
}

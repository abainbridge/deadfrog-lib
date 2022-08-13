#include "df_bitmap.h" 
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include <math.h>
#include <stdlib.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#define M_PI 3.1415926535


enum { SCREEN_WIDTH = 1024 };
enum { SCREEN_HEIGHT = 768 };

enum ObjectType {
    UNDEFINED,
    SHIP,
    BULLET,
    BIG_ASTEROID
};


struct Vec2 {
    double x, y;
};

struct Object {
    ObjectType type;
    double x, y;
    double velX, velY;
    double angleRadians;
    double age;
};


Object g_ship = {
    SHIP,
    SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, // x, y
    0.0, 0.0, // vel
    1.5708, // angleRadians
    0.0 // age
};

Object g_bullets[4];

Object g_asteroids[4];


// ****************************************************************************
// Misc functions
// ****************************************************************************

void WrapPosition(Object *obj) {
    if (obj->x < 0.0)
        obj->x += SCREEN_WIDTH;
    else if (obj->x > SCREEN_WIDTH)
        obj->x -= SCREEN_WIDTH;
    if (obj->y < 0.0)
        obj->y += SCREEN_HEIGHT;
    else if (obj->y > SCREEN_HEIGHT)
        obj->y -= SCREEN_HEIGHT;
}

void RotateVerts(Vec2 *verts, int numVerts, double angleRadians) {
    double _sin = sin(angleRadians);
    double _cos = cos(angleRadians);
    for (int i = 0; i < numVerts; i++) {
        double tmp = verts[i].x * _cos - verts[i].y * _sin;
        verts[i].y = verts[i].x * _sin + verts[i].y * _cos;
        verts[i].x = tmp;
    }
}

void RenderLinePath(DfBitmap *bmp, double xOffset, double yOffset,
    Vec2 *verts, int numVerts) {
    double x = verts[0].x + xOffset;
    double y = verts[0].y + yOffset;
    for (int i = 1; i < numVerts; i++) {
        double nextX = verts[i].x + xOffset;
        double nextY = verts[i].y + yOffset;
        DrawLine(bmp, x, y, nextX, nextY, g_colourWhite);
        x = nextX;
        y = nextY;
    }
}


// ****************************************************************************
// Bullet
// ****************************************************************************

void FireBullet() {
    int i;
    for (i = 0; i < ARRAY_SIZE(g_bullets); i++)
    if (g_bullets[i].type == UNDEFINED) break;

    if (i == ARRAY_SIZE(g_bullets)) return;

    double const BULLET_SPEED = 430.0;
    g_bullets[i].type = BULLET;
    g_bullets[i].x = g_ship.x;
    g_bullets[i].y = g_ship.y;
    g_bullets[i].velX = g_ship.velX + sin(g_ship.angleRadians) * BULLET_SPEED;
    g_bullets[i].velY = g_ship.velY - cos(g_ship.angleRadians) * BULLET_SPEED;
    g_bullets[i].age = 0.0;
}

void AdvanceBullet(Object *obj, double advanceTime) {
    obj->age += advanceTime;
    double const BULLET_LIFE = 1.1;
    if (obj->age > BULLET_LIFE) {
        obj->type = UNDEFINED;
        return;
    }

    obj->x += obj->velX * advanceTime;
    obj->y += obj->velY * advanceTime;
    WrapPosition(obj);
}

void RenderBullet(DfBitmap *bmp, Object *obj) {
    if (obj->type != UNDEFINED)
        PutPix(bmp, obj->x, obj->y, g_colourWhite);
}


// ****************************************************************************
// Ship
// ****************************************************************************

void AdvanceShip(DfWindow *win, Object *obj) {
    if (win->input.keys[KEY_LEFT])
        obj->angleRadians -= 6.3 * win->advanceTime;
    if (win->input.keys[KEY_RIGHT])
        obj->angleRadians += 6.3 * win->advanceTime;
    if (win->input.keys[KEY_UP]) {
        double const thrustForce = 5.5;
        obj->velX += sin(obj->angleRadians) * thrustForce;
        obj->velY -= cos(obj->angleRadians) * thrustForce;
    }
    if (win->input.keyDowns[KEY_SPACE])
        FireBullet();

    double const frictionCoef = 0.21;
    obj->velX -= obj->velX * win->advanceTime * frictionCoef;
    obj->velY -= obj->velY * win->advanceTime * frictionCoef;
    obj->x += obj->velX * win->advanceTime;
    obj->y += obj->velY * win->advanceTime;

    if (win->input.keys[KEY_UP])
        obj->age += win->advanceTime;
    else
        obj->age = 0.0;

    WrapPosition(obj);
}

void RenderShip(DfBitmap *bmp, Object *obj) {
    enum { NUM_VERTS = 6 };
    Vec2 shipverts[NUM_VERTS] = { 
        { 0.0, -12.0 },
        { 6.5, 9.0 },
        { 3.5, 6.0 },
        { -3.5, 6.0 },
        { -6.5, 9.0 },
        { 0.0, -12.0 }
    };

    RotateVerts(shipverts, NUM_VERTS, obj->angleRadians);
    RenderLinePath(bmp, obj->x, obj->y, shipverts, NUM_VERTS);

    // Render rocket thrust.
    int intAge = obj->age * 20.0;
    if (intAge & 1) {
        Vec2 rocketverts[] = {
            { 3.5, 8.0 },
            { 0.0, 12.0 },
            { -3.5, 8.0 }
        };

        RotateVerts(rocketverts, ARRAY_SIZE(rocketverts), obj->angleRadians);
        RenderLinePath(bmp, obj->x, obj->y, rocketverts, ARRAY_SIZE(rocketverts));
    }
}


// ****************************************************************************
// Asteroid
// ****************************************************************************

void AdvanceAsteroid(Object *obj, double advanceTime) {
    obj->x += obj->velX * advanceTime;
    obj->y += obj->velY * advanceTime;
    WrapPosition(obj);
}

void RenderAsteroid(DfBitmap *bmp, Object *obj) {
    Vec2 verts[12];
    srand((unsigned)obj);
    double angle = 0.0;
    double angleIncrement = M_PI * 2.0 / (ARRAY_SIZE(verts) - 1);
    double avgRadius = 30.0;
    int radiusRandAmount = 20;
    for (int i = 0; i < ARRAY_SIZE(verts) - 1; i++) {
        double dist = avgRadius + rand() % radiusRandAmount;
        verts[i].x = sin(angle) * dist;
        verts[i].y = cos(angle) * dist;
        angle += angleIncrement;
    }
    verts[ARRAY_SIZE(verts) - 1].x = verts[0].x;
    verts[ARRAY_SIZE(verts) - 1].y = verts[0].y;

    RotateVerts(verts, ARRAY_SIZE(verts), obj->angleRadians);
    RenderLinePath(bmp, obj->x, obj->y, verts, ARRAY_SIZE(verts));
}


// ****************************************************************************
// Main
// ****************************************************************************

void AsteroidsMain() {
    DfWindow *win = CreateWin(SCREEN_WIDTH, SCREEN_HEIGHT, WT_WINDOWED_RESIZEABLE, "Asteroids Example");

    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));
    for (int i = 0; i < ARRAY_SIZE(g_asteroids); i++) {
        g_asteroids[i].type = BIG_ASTEROID;
        g_asteroids[i].x = rand() % SCREEN_WIDTH;
        g_asteroids[i].y = rand() % SCREEN_HEIGHT;
        g_asteroids[i].velX = rand() % 100;
        g_asteroids[i].velY = rand() % 100;
        g_asteroids[i].angleRadians = 0.0;
    }

    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        BitmapClear(win->bmp, g_colourBlack);
        InputPoll(win);

        AdvanceShip(win, &g_ship);
        for (int i = 0; i < ARRAY_SIZE(g_bullets); i++)
            AdvanceBullet(&g_bullets[i], win->advanceTime);
        for (int i = 0; i < ARRAY_SIZE(g_asteroids); i++)
            AdvanceAsteroid(&g_asteroids[i], win->advanceTime);

        RenderShip(win->bmp, &g_ship);
        for (int i = 0; i < ARRAY_SIZE(g_bullets); i++)
            RenderBullet(win->bmp, &g_bullets[i]);
        for (int i = 0; i < ARRAY_SIZE(g_asteroids); i++)
            RenderAsteroid(win->bmp, &g_asteroids[i]);

        // Draw frames per second counter
        DrawTextRight(g_defaultFont, g_colourWhite, win->bmp, win->bmp->width - 5, 0, "FPS:%i", win->fps);

        UpdateWin(win);
        WaitVsync();
    }
}

#include "df_bitmap.h" 
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#define M_PI 3.1415926535
double const BULLET_LIFE_SECONDS = 1.1;
double const DEATH_ANIM_SECONDS = 3.0;
enum { SHIP_NUM_VERTS = 6 };
enum { ASTEROID_NUM_VERTS = 13 };

enum { SCREEN_WIDTH = 1024 };
enum { SCREEN_HEIGHT = 768 };

enum GameState {
    PLAYING,
    PLAYER_DYING
};


struct Vec2 {
    double x, y;
};

struct Ship {
    Vec2 pos;
    Vec2 vel;
    double angleRadians;
    double thrustSeconds;
    Vec2 vertsWorldSpace[SHIP_NUM_VERTS];
};

struct Bullet {
    Vec2 pos;
    Vec2 vel;
    double age;
};

struct Asteroid {
    Vec2 pos;
    Vec2 vel;
    bool exists;
    Vec2 vertsWorldSpace[ASTEROID_NUM_VERTS];
};


Ship g_ship;
Bullet g_bullets[4];
Asteroid g_asteroids[4];
GameState g_gameState = PLAYING;
double g_playerDeathAnimTime;


// ****************************************************************************
// Misc functions
// ****************************************************************************

void WrapPosition(Vec2 *pos) {
    if (pos->x < 0.0)
        pos->x += SCREEN_WIDTH;
    else if (pos->x > SCREEN_WIDTH)
        pos->x -= SCREEN_WIDTH;
    if (pos->y < 0.0)
        pos->y += SCREEN_HEIGHT;
    else if (pos->y > SCREEN_HEIGHT)
        pos->y -= SCREEN_HEIGHT;
}

void RotateAndTranslateVerts(Vec2 *verts, int numVerts, double angleRadians, Vec2 translation) {
    double _sin = sin(angleRadians);
    double _cos = cos(angleRadians);
    for (int i = 0; i < numVerts; i++) {
        double tmp = verts[i].x * _cos - verts[i].y * _sin;
        verts[i].y = verts[i].x * _sin + verts[i].y * _cos;
        verts[i].x = tmp + translation.x;
        verts[i].y += translation.y;
    }
}

void RenderLinePath(DfBitmap *bmp, Vec2 *verts, int numVerts, DfColour col) {
    double x = verts[0].x;
    double y = verts[0].y;
    for (int i = 1; i < numVerts; i++) {
        double nextX = verts[i].x;
        double nextY = verts[i].y;
        DrawLine(bmp, x, y, nextX, nextY, col);
        x = nextX;
        y = nextY;
    }
}

void RenderLinePathExploding(DfBitmap *bmp, Vec2 posOffset, Vec2 *verts, int numVerts, double age) {
    double fractionComplete = age / DEATH_ANIM_SECONDS;
    if (fractionComplete > 1.0) return;
    double explosionFactor = age * 5.0;
    double x = verts[0].x;
    double y = verts[0].y;
    double skipLut[] = { 0.8, 0.35, 0.5, 0.4, 0.25, 0.3, 0.6, 0.9 };
    for (int i = 1; i < numVerts; i++) {
        double nextX = verts[i].x;
        double nextY = verts[i].y;
        if (fractionComplete < skipLut[i]) {
            double midX = (nextX + x) * 0.5;
            double midY = (nextY + y) * 0.5;
            double moveX = explosionFactor * midX + posOffset.x;
            double moveY = explosionFactor * midY + posOffset.y;
            DrawLine(bmp, x + moveX, y + moveY, nextX + moveX, nextY + moveY, g_colourWhite);
        }
        x = nextX;
        y = nextY;
    }
}

// Run a semi-infinite ray horizontally (increasing x) from the test point and
// count how many edges it crosses. At each crossing, the ray switches between
// inside and outside. This is called the Jordan curve theorem.
bool PointInPolygon(Vec2 *verts, int numVerts, Vec2 point) {
    bool c = false;
    for (int i = 0, j = numVerts - 1; i < numVerts; j = i++) {
        if ((verts[i].y > point.y) == (verts[j].y > point.y))
            continue; // Edge doesn't intersect our ray at all.

        double intersectionX = (verts[j].x - verts[i].x) * (point.y - verts[i].y) / (verts[j].y - verts[i].y) + verts[i].x;
        if (point.x < intersectionX)
            c = !c;
    }
    return c;
}


// ****************************************************************************
// Bullet
// ****************************************************************************

void InitBullets() {
    for (int i = 0; i < ARRAY_SIZE(g_bullets); i++) {
        g_bullets[i].age = 9e9;
    }
}

void FireBullet() {
    int i;
    for (i = 0; i < ARRAY_SIZE(g_bullets); i++)
    if (g_bullets[i].age > BULLET_LIFE_SECONDS) break;

    if (i == ARRAY_SIZE(g_bullets)) return;

    double const BULLET_SPEED = 550.0;
    g_bullets[i].pos.x = g_ship.pos.x;
    g_bullets[i].pos.y = g_ship.pos.y;
    g_bullets[i].vel.x = g_ship.vel.x + sin(g_ship.angleRadians) * BULLET_SPEED;
    g_bullets[i].vel.y = g_ship.vel.y - cos(g_ship.angleRadians) * BULLET_SPEED;
    g_bullets[i].age = 0.0;
}

void AdvanceBullet(Bullet *bul, double advanceTime) {
    bul->age += advanceTime;
    if (bul->age > BULLET_LIFE_SECONDS) {
        return;
    }

    bul->pos.x += bul->vel.x * advanceTime;
    bul->pos.y += bul->vel.y * advanceTime;
    WrapPosition(&bul->pos);
}

void RenderBullet(DfBitmap *bmp, Bullet *bul) {
    if (bul->age < BULLET_LIFE_SECONDS)
        PutPix(bmp, bul->pos.x, bul->pos.y, g_colourWhite);
}


// ****************************************************************************
// Ship
// ****************************************************************************

void InitShip() {
    g_ship.pos.x = SCREEN_WIDTH / 2;
    g_ship.pos.y = SCREEN_HEIGHT / 2;
    g_ship.vel.x = g_ship.vel.y = 0.0;
    g_ship.angleRadians = 1.5708;
    g_ship.thrustSeconds = 0.0;
}

void AdvanceShip(DfWindow *win) {
    if (g_gameState == PLAYING) {
        double const rotationRate = 4.5;
        if (win->input.keys[KEY_LEFT])
            g_ship.angleRadians -= rotationRate * win->advanceTime;
        if (win->input.keys[KEY_RIGHT])
            g_ship.angleRadians += rotationRate * win->advanceTime;
        if (win->input.keys[KEY_UP]) {
            double const thrustForce = 4.0;
            g_ship.vel.x += sin(g_ship.angleRadians) * thrustForce;
            g_ship.vel.y -= cos(g_ship.angleRadians) * thrustForce;
        }
        if (win->input.keyDowns[KEY_SPACE])
            FireBullet();
        if (win->input.keyDowns[KEY_D]) {
            g_gameState = PLAYER_DYING;
            g_playerDeathAnimTime = 0.0;
        }
        if (win->input.keys[KEY_UP])
            g_ship.thrustSeconds += win->advanceTime;
        else
            g_ship.thrustSeconds = 0.0;
    }
    else {
        g_playerDeathAnimTime += win->advanceTime;
    }

    double const frictionCoef = 0.27;
    g_ship.vel.x -= g_ship.vel.x * win->advanceTime * frictionCoef;
    g_ship.vel.y -= g_ship.vel.y * win->advanceTime * frictionCoef;
    g_ship.pos.x += g_ship.vel.x * win->advanceTime;
    g_ship.pos.y += g_ship.vel.y * win->advanceTime;

    WrapPosition(&g_ship.pos);

    static Vec2 const shipverts[SHIP_NUM_VERTS] = {
        { 0.0, -14.0 },
        { 7.5, 10.5 },
        { 4.5, 7.0 },
        { -4.5, 7.0 },
        { -7.5, 10.5 },
        { 0.0, -14.0 }
    };

    memcpy(g_ship.vertsWorldSpace, shipverts, sizeof(shipverts));
    RotateAndTranslateVerts(g_ship.vertsWorldSpace, SHIP_NUM_VERTS, g_ship.angleRadians, g_ship.pos);
}

void RenderShip(DfBitmap *bmp) {
    if (g_gameState == PLAYER_DYING)
        RenderLinePathExploding(bmp, g_ship.pos, g_ship.vertsWorldSpace, SHIP_NUM_VERTS, g_playerDeathAnimTime);
    else
        RenderLinePath(bmp, g_ship.vertsWorldSpace, SHIP_NUM_VERTS, g_colourWhite);

    // Render rocket thrust.
    int intAge = g_ship.thrustSeconds * 20.0;
    if (intAge & 1) {
        Vec2 rocketverts[] = {
            { 3.5, 8.0 },
            { 0.0, 14.0 },
            { -3.5, 8.0 }
        };

        RotateAndTranslateVerts(rocketverts, ARRAY_SIZE(rocketverts), g_ship.angleRadians, g_ship.pos);
        RenderLinePath(bmp, rocketverts, ARRAY_SIZE(rocketverts), g_colourWhite);
    }
}


// ****************************************************************************
// Asteroid
// ****************************************************************************

void InitAsteroids() {
    for (int i = 0; i < ARRAY_SIZE(g_asteroids); i++) {
        g_asteroids[i].pos.x = rand() % SCREEN_WIDTH;
        g_asteroids[i].pos.y = rand() % SCREEN_HEIGHT;
        g_asteroids[i].vel.x = rand() % 100;
        g_asteroids[i].vel.y = rand() % 100;
        g_asteroids[i].exists = true;
    }
}

void AdvanceAsteroid(Asteroid *ast, double advanceTime) {
    ast->pos.x += ast->vel.x * advanceTime;
    ast->pos.y += ast->vel.y * advanceTime;
    WrapPosition(&ast->pos);

    static Vec2 const verts[4][ASTEROID_NUM_VERTS] = {
        { { 4, 0 }, { 6, 0 }, { 8, 3 }, { 8, 5 }, { 6, 8 }, { 4, 8 }, { 4, 5 }, { 2, 8 }, { 0, 5 }, { 2, 4 }, { 0, 3 }, { 3, 0 }, { 4, 0 } },
        { { 5, 0 }, { 8, 2 }, { 8, 3 }, { 5, 4 }, { 8, 6 }, { 6, 8 }, { 5, 7 }, { 2, 8 }, { 0, 5 }, { 0, 2 }, { 3, 2 }, { 2, 0 }, { 5, 0 } },
        { { 6, 0 }, { 8, 2 }, { 7, 4 }, { 8, 6 }, { 5, 8 }, { 2, 8 }, { 0, 6 }, { 0, 5 }, { 0, 3 }, { 0, 2 }, { 2, 0 }, { 4, 2 }, { 6, 0 } },
        { { 6, 0 }, { 8, 2 }, { 6, 3 }, { 8, 5 }, { 6, 8 }, { 3, 7 }, { 2, 8 }, { 0, 6 }, { 1, 4 }, { 0, 2 }, { 2, 0 }, { 4, 1 }, { 6, 0 } },
    };

    for (int i = 0; i < ASTEROID_NUM_VERTS; i++) {
        ast->vertsWorldSpace[i].x = verts[3][i].x * 9.0 + ast->pos.x;
        ast->vertsWorldSpace[i].y = verts[3][i].y * 9.0 + ast->pos.y;
    }

    for (int i = 0; i < ARRAY_SIZE(g_bullets); i++) {
        if (g_bullets[i].age > BULLET_LIFE_SECONDS)
            return;
        if (PointInPolygon(ast->vertsWorldSpace, ASTEROID_NUM_VERTS, g_bullets[i].pos)) {
            ast->exists = false;
            g_bullets[i].age = BULLET_LIFE_SECONDS + 1.0;
        }
    }
}

void RenderAsteroid(DfBitmap *bmp, Asteroid *ast) {
    if (!ast->exists) return;

    DfColour grey = { 0xff999999 };
    RenderLinePath(bmp, ast->vertsWorldSpace, ASTEROID_NUM_VERTS, grey);
}


// ****************************************************************************
// Game State
// ****************************************************************************

void AdvanceGameState() {
    if (g_gameState == PLAYER_DYING) {
        if (g_playerDeathAnimTime > DEATH_ANIM_SECONDS) {
            InitShip();
            g_gameState = PLAYING;
        }
    }
}


// ****************************************************************************
// Main
// ****************************************************************************

void AsteroidsMain() {
    DfWindow *win = CreateWin(SCREEN_WIDTH, SCREEN_HEIGHT, WT_WINDOWED_RESIZEABLE, "Asteroids Example");

    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    InitShip();
    InitAsteroids();
    InitBullets();

    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        BitmapClear(win->bmp, g_colourBlack);
        InputPoll(win);

        AdvanceGameState();
        AdvanceShip(win);
        for (int i = 0; i < ARRAY_SIZE(g_bullets); i++)
            AdvanceBullet(&g_bullets[i], win->advanceTime);
        for (int i = 0; i < ARRAY_SIZE(g_asteroids); i++)
            AdvanceAsteroid(&g_asteroids[i], win->advanceTime);

        RenderShip(win->bmp);
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

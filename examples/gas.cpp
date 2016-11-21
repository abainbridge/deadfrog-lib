// Simulates an ideal gas with about 1000 particles.
// The physics for this simulation came from:
// http://vobarian.com/collisions/2dcollisions2.pdf

#include "df_bmp.h"
#include "df_font.h"
#include "df_time.h"
#include "df_window.h"

#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>


struct Particle
{
    float x, y;
    float vx, vy;
};


static const unsigned NUM_PARTICLES = 1000;
static Particle g_points[NUM_PARTICLES];

static const unsigned SPEED_HISTOGRAM_NUM_BINS = 20;
static unsigned g_speedHistogram[SPEED_HISTOGRAM_NUM_BINS];

static unsigned const GRID_ROWS = 20;
static unsigned const GRID_LINES = 20;
static unsigned g_grid[GRID_LINES * GRID_ROWS];

static float const MAX_INITIAL_SPEED = 50.0f;
static float const RADIUS = 3.0f;


static float frand(float range)
{
    return ((float)rand() / (float)RAND_MAX) * range;
}


static void InitParticles()
{
    for (unsigned i = 0; i < NUM_PARTICLES; i++)
    {
        g_points[i].x = frand(g_window->bmp->width);
        g_points[i].y = frand(g_window->bmp->height);
        g_points[i].vx = MAX_INITIAL_SPEED;
        g_points[i].vy = MAX_INITIAL_SPEED;
    }
}


static float DotProduct(float ax, float ay, float bx, float by)
{
    return ax * bx + ay * by;
}


static inline bool isPositive(float x)
{
    return x > 0.0f;
}


static void Advance()
{
    static double prevTime = DfGetTime();
    double now = DfGetTime();
    float advanceTime = now - prevTime;
    if (advanceTime > 0.1f)
        advanceTime = 0.1f;
    prevTime = now;

    float const WORLD_SIZE_X = g_window->bmp->width;
    float const WORLD_SIZE_Y = g_window->bmp->height;

    for (unsigned i = 0; i < NUM_PARTICLES; i++)
    {
        Particle *p = g_points + i;
        p->x += p->vx * advanceTime;
        if ((p->x < 0.0f && p->vx < 0.0f) || (p->x > WORLD_SIZE_X && p->vx > 0.0f))
            p->vx = -p->vx;

        p->y += p->vy * advanceTime;
        if ((p->y < 0.0f && p->vy < 0.0f) || (p->y > WORLD_SIZE_Y && p->vy > 0.0f))
            p->vy = -p->vy;
    }

    memset(g_speedHistogram, 0, sizeof(unsigned) * SPEED_HISTOGRAM_NUM_BINS);

    float const RADIUS2 = RADIUS * 2;
    for (unsigned i = 0; i < NUM_PARTICLES; i++)
    {
        Particle *p1 = g_points + i;
        for (unsigned j = 0; j < i; j++)
        {
            Particle *p2 = g_points + j;
            float deltaX = p2->x - p1->x;
            if (deltaX > RADIUS2)
                continue;
            float deltaY = p2->y - p1->y;
            if (deltaY > RADIUS2)
                continue;
            float distSqrd = deltaX * deltaX + deltaY * deltaY;
            if (distSqrd < RADIUS2 * RADIUS2)
            {
                // There has been a collision.

                // Collision normal is the vector between the two particle centers.
                // The only change in velocity of either particle is in the 
                // direction of the collision normal.

                // Calculate collision unit normal and tangent.
                float dist = sqrtf(distSqrd);
                float invDist = 1.0f / dist;
                float normX = deltaX * invDist;
                float normY = deltaY * invDist;
                float tangX = normY;
                float tangY = -normX;

                // Project p1 and p2 velocities onto collision unit normal
                float p1VelNorm = DotProduct(p1->vx, p1->vy, normX, normY);
                float p2VelNorm = DotProduct(p2->vx, p2->vy, normX, normY);

                // Project p1 and p2 velocities onto collision tangent
                float p1VelTang = DotProduct(p1->vx, p1->vy, tangX, tangY);
                float p2VelTang = DotProduct(p2->vx, p2->vy, tangX, tangY);

                // Since all particles have the same mass, the collision
                // simply swaps the particles velocities along the normal.
                // This a no-op. We just swap the variables we use in the
                // calculations below.

                // Convert the scalar normal and tangential velocities back
                // into vectors.
                float p1VelNormX = p2VelNorm * normX;
                float p1VelNormY = p2VelNorm * normY;
                float p1VelTangX = p1VelTang * tangX;
                float p1VelTangY = p1VelTang * tangY;

                float p2VelNormX = p1VelNorm * normX;
                float p2VelNormY = p1VelNorm * normY;
                float p2VelTangX = p2VelTang * tangX;
                float p2VelTangY = p2VelTang * tangY;

                // The final velocities of the particles are now just the
                // sums of the normal and tangential velocity vectors
                p1->vx = p1VelNormX + p1VelTangX;
                p1->vy = p1VelNormY + p1VelTangY;
                p2->vx = p2VelNormX + p2VelTangX;
                p2->vy = p2VelNormY + p2VelTangY;

                // Now move the two particles apart by a few percent of their 
                // embeddedness to prevent the pathological interactions that
                // can occur causing the two particles keep re-colliding every
                // frame.
                float embeddedness = RADIUS2 - dist;
                float pushAmount = embeddedness * 0.02f;
                p1->x -= normX * pushAmount;
                p1->y -= normY * pushAmount;
                p2->x += normX * pushAmount;
                p2->y += normY * pushAmount;
            }
        }

        {
            float speed = sqrtf(p1->vx * p1->vx + p1->vy * p1->vy);
            unsigned bin_index = SPEED_HISTOGRAM_NUM_BINS * speed / (3.0f * MAX_INITIAL_SPEED);
            if (bin_index >= SPEED_HISTOGRAM_NUM_BINS)
                bin_index = SPEED_HISTOGRAM_NUM_BINS - 1;
            g_speedHistogram[bin_index]++;
        }
    }
}


static void Render()
{
    for (unsigned i = 0; i < NUM_PARTICLES; i++)
    {
        Particle *p = g_points + i;
        CircleFill(g_window->bmp, p->x, p->y, RADIUS, g_colourWhite);
    }

    for (unsigned i = 0; i < SPEED_HISTOGRAM_NUM_BINS; i++)
    {
        const unsigned barWidth = 4;
        const float scale = 1000.0f / (float)NUM_PARTICLES;
        unsigned h = g_speedHistogram[i] * scale;
        RectFill(g_window->bmp, i * barWidth, g_window->bmp->height - h, barWidth, h, Colour(255, 99, 99));
    }
}


void GasMain()
{
    CreateWin(800, 600, WT_WINDOWED, "Ideal Gas Simulation Example");
    DfFont *font = DfCreateFont("Courier New", 12, 4);

    InitParticles();

    unsigned frameNum = 0;
    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        ClearBitmap(g_window->bmp, g_colourBlack);
        InputManagerAdvance();

        Advance();
        Render();

        // Draw frames per second counter
        DrawTextRight(font, g_colourWhite, g_window->bmp, g_window->bmp->width - 5, 0, "FPS:%i", g_window->fps);

        UpdateWin();
        DfSleepMillisec(10);
    }
}

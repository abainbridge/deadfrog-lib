// Simulates a 2D viscous fluid using the Lattice Boltzmann Method. It models
// a stream of fluid flowing past a barrier. The flow starts off laminar and
// gradually becomes turbulent. The initial disturbances needed to allow the
// turbulence to form come from the errors introduced by the floating point
// arithmetic.
//
// Based on https://physics.weber.edu/schroeder/fluids/
#include "df_font.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include <math.h>
#include <stdlib.h>


struct Cell {
    float n0;
    float nN, nS, nE, nW;
    float nNE, nSE, nNW, nSW;
    float ux, uy;
    bool barrier;
};

enum { X_DIM = 400 };
enum { Y_DIM = 160 };
enum { RENDER_SCALE = 3 }; // Width of cell in pixels.
enum { NUM_TRACERS = 144 };
static const float fluid_speed = 0.1f;
static const float viscosity = 0.02f;
static const float four9ths = 4.0f / 9.0f;					// abbreviations
static const float one9th = 1.0f / 9.0f;
static const float one36th = 1.0f / 36.0f;

// Index into this 2D array using x + y*X_DIM, traversing rows first and then columns.
static Cell g_cells[X_DIM * Y_DIM];
float g_tracers_x[NUM_TRACERS];
float g_tracers_y[NUM_TRACERS];


static Cell *cell(int x, int y) {
    return &g_cells[x + y * X_DIM];
}


// Set all densities in a cell to their equilibrium values.
void SetEquilibrium(int x, int y) {
    float new_ux = fluid_speed;
    float ux3 = 3 * new_ux;
    float uy3 = 0.0;
    float ux2 = new_ux * new_ux;
    float uy2 = 0.0;
    float uxuy2 = 0.0;
    float u2 = ux2 + uy2;
    float u215 = 1.5 * u2;
    Cell *c = cell(x, y);
    c->n0 = four9ths * (1 - u215);
    c->nE = one9th * (1 + ux3 + 4.5 * ux2 - u215);
    c->nW = one9th * (1 - ux3 + 4.5 * ux2 - u215);
    c->nN = one9th * (1 + uy3 + 4.5 * uy2 - u215);
    c->nS = one9th * (1 - uy3 + 4.5 * uy2 - u215);
    c->nNE = one36th * (1 + ux3 + uy3 + 4.5 * (u2 + uxuy2) - u215);
    c->nSE = one36th * (1 + ux3 - uy3 + 4.5 * (u2 - uxuy2) - u215);
    c->nNW = one36th * (1 - ux3 + uy3 + 4.5 * (u2 - uxuy2) - u215);
    c->nSW = one36th * (1 - ux3 - uy3 + 4.5 * (u2 + uxuy2) - u215);
    c->ux = new_ux;
    c->uy = 0.0;
}


// Initialize or re-initialize the fluid, based on speed slider setting:
void InitCells() {
    for (int y = 0; y < Y_DIM; y++) {
        for (int x = 0; x < X_DIM; x++) {
            SetEquilibrium(x, y);
        }
    }

    // Create a simple linear "wall" barrier (intentionally a little offset from center).
    int barrier_size = 8;
    for (int y = (Y_DIM / 2) - barrier_size; y <= (Y_DIM / 2) + barrier_size; y++) {
        int x = Y_DIM / 3.0;
        cell(x, y)->barrier = true;
    }
}


// Collide particles within each cell (here's the physics!).
void Collide() {
    float omega = 1.0 / (3.0 * viscosity + 0.5);		// reciprocal of relaxation time
    for (int y = 1; y < Y_DIM - 1; y++) {
        for (int x = 1; x < X_DIM - 1; x++) {
            Cell *c = cell(x, y);
            float this_rho = c->n0 + c->nN + c->nS + c->nE + c->nW + c->nNW + c->nNE + c->nSW + c->nSE;
            float this_ux = (c->nE + c->nNE + c->nSE - c->nW - c->nNW - c->nSW) / this_rho;
            c->ux = this_ux;
            float this_uy = (c->nN + c->nNE + c->nNW - c->nS - c->nSE - c->nSW) / this_rho;
            c->uy = this_uy;
            float one9th_rho = one9th * this_rho;		// pre-compute a bunch of stuff for optimization
            float one36th_rho = one36th * this_rho;
            float ux3 = 3 * this_ux;
            float uy3 = 3 * this_uy;
            float ux2 = this_ux * this_ux;
            float uy2 = this_uy * this_uy;
            float uxuy2 = 2 * this_ux * this_uy;
            float u2 = ux2 + uy2;
            float u215 = 1.5 * u2;
            c->n0 += omega * (four9ths * this_rho * (1 - u215) - c->n0);
            c->nE += omega * (one9th_rho * (1 + ux3 + 4.5*ux2 - u215) - c->nE);
            c->nW += omega * (one9th_rho * (1 - ux3 + 4.5*ux2 - u215) - c->nW);
            c->nN += omega * (one9th_rho * (1 + uy3 + 4.5*uy2 - u215) - c->nN);
            c->nS += omega * (one9th_rho * (1 - uy3 + 4.5*uy2 - u215) - c->nS);
            c->nNE += omega * (one36th_rho * (1 + ux3 + uy3 + 4.5*(u2 + uxuy2) - u215) - c->nNE);
            c->nSE += omega * (one36th_rho * (1 + ux3 - uy3 + 4.5*(u2 - uxuy2) - u215) - c->nSE);
            c->nNW += omega * (one36th_rho * (1 - ux3 + uy3 + 4.5*(u2 - uxuy2) - u215) - c->nNW);
            c->nSW += omega * (one36th_rho * (1 - ux3 - uy3 + 4.5*(u2 + uxuy2) - u215) - c->nSW);
        }
    }

    for (int y = 1; y < Y_DIM - 2; y++) {
        cell(X_DIM - 1, y)->nW = cell(X_DIM - 2, y)->nW;    // at right end, copy left-flowing densities from next row to the left
        cell(X_DIM - 1, y)->nNW = cell(X_DIM - 2, y)->nNW;
        cell(X_DIM - 1, y)->nSW = cell(X_DIM - 2, y)->nSW;
    }
}


// Move particles along their directions of motion.
void Stream() {
    for (int y = Y_DIM - 2; y > 0; y--) {			// first start in NW corner...
        for (int x = 1; x < X_DIM - 1; x++) {
            cell(x, y)->nN = cell(x, y - 1)->nN;			// move the north-moving particles
            cell(x, y)->nNW = cell(x + 1, y - 1)->nNW;		// and the northwest-moving particles
        }
    }
    for (int y = Y_DIM - 2; y > 0; y--) {			// now start in NE corner...
        for (int x = X_DIM - 2; x > 0; x--) {
            cell(x, y)->nE = cell(x - 1, y)->nE;			// move the east-moving particles
            cell(x, y)->nNE = cell(x - 1 , y - 1)->nNE;		// and the northeast-moving particles
        }
    }
    for (int y = 1; y < Y_DIM - 1; y++) {			// now start in SE corner...
        for (int x = X_DIM - 2; x > 0; x--) {
            cell(x, y)->nS = cell(x, y + 1)->nS;			// move the south-moving particles
            cell(x, y)->nSE = cell(x - 1, y + 1)->nSE;		// and the southeast-moving particles
        }
    }
    for (int y = 1; y < Y_DIM - 1; y++) {				// now start in the SW corner...
        for (int x = 1; x < X_DIM - 1; x++) {
            cell(x, y)->nW = cell(x + 1, y)->nW;			// move the west-moving particles
            cell(x, y)->nSW = cell(x + 1, y + 1)->nSW;		// and the southwest-moving particles
        }
    }
    for (int y = 1; y < Y_DIM - 1; y++) {				// Now handle bounce-back from barriers
        for (int x = 1; x < X_DIM - 1; x++) {
            if (cell(x, y)->barrier) {
                cell(x + 1, y)->nE = cell(x, y)->nW;
                cell(x - 1, y)->nW = cell(x, y)->nE;
                cell(x, y + 1)->nN = cell(x, y)->nS;
                cell(x, y - 1)->nS = cell(x, y)->nN;
                cell(x + 1, y + 1)->nNE = cell(x, y)->nSW;
                cell(x - 1, y + 1)->nNW = cell(x, y)->nSE;
                cell(x + 1, y - 1)->nSE = cell(x, y)->nNW;
                cell(x - 1, y - 1)->nSW = cell(x, y)->nNE;
            }
        }
    }
}


void InitTracers() {
    for (int i = 0; i < NUM_TRACERS; i++) {
        g_tracers_x[i] = X_DIM / (float)NUM_TRACERS * i;
        g_tracers_y[i] = (float)rand() / (float)RAND_MAX * Y_DIM;
    }
}


void MoveTracers() {
    for (int i = 0; i < NUM_TRACERS; i++) {
        Cell *c = cell(g_tracers_x[i], g_tracers_y[i]);
        g_tracers_x[i] += c->ux;
        g_tracers_y[i] += c->uy;
        if (g_tracers_x[i] > X_DIM - 1) {
            g_tracers_x[i] = 0.0;
            g_tracers_y[i] = (float)rand() / (float)RAND_MAX * Y_DIM;
        }
    }
}


void DrawTracers(DfWindow *win) {
    for (int i = 0; i < NUM_TRACERS; i++) {
        RectFill(win->bmp, g_tracers_x[i] * RENDER_SCALE, g_tracers_y[i] * RENDER_SCALE, 
            RENDER_SCALE, RENDER_SCALE, g_colourBlack);
    }
}


void Draw(DfWindow *win) {
    BitmapClear(win->bmp, g_colourWhite);

    for (int y = 1; y < Y_DIM - 1; y++) {
        for (int x = 1; x < X_DIM - 1; x++) {
            DfColour col = g_colourBlack;

            if (!cell(x, y)->barrier) {
                // Compute the curl (actually times 2) of the macroscopic velocity field.
                float curl = cell(x + 1, y)->uy -
                    cell(x - 1, y)->uy -
                    cell(x, y + 1)->ux +
                    cell(x, y - 1)->ux;
                
                float contrast = 2000.0;
                col = g_colourWhite;
                int tmp = ClampInt(abs(curl * contrast), 0, 255);
                if (curl < 0.0) {
                    col.g = 255 - tmp;
                    col.r = col.g;
                }
                else {
                    col.g = 255 - tmp;
                    col.b = col.g;
                }
            }

            RectFill(win->bmp, x * RENDER_SCALE, y * RENDER_SCALE,
                RENDER_SCALE, RENDER_SCALE, col);
        }
    }

    DrawTracers(win);
}


void Simulate() {
    for (int step = 0; step < 10; step++) {
        Collide();
        Stream();
        MoveTracers();
    }
}


void LatticeBoltzmannMain() {
    DfWindow *win = CreateWin(X_DIM * RENDER_SCALE, Y_DIM * RENDER_SCALE, WT_WINDOWED_RESIZEABLE,
        "Lattice Boltzmann Method Fluid Simulation Example");
    g_defaultFont = LoadFontFromMemory(df_mono_7x13, sizeof(df_mono_7x13));

    InitCells();
    InitTracers();

    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        InputPoll(win);

        Simulate();
        Draw(win);

        // Draw frames per second counter
        RectFill(win->bmp, win->bmp->width - 55, 0, 55, g_defaultFont->charHeight + 2, g_colourBlack);
        DrawTextLeft(g_defaultFont, g_colourWhite, win->bmp, win->bmp->width - 50, 0, "FPS:%i", win->fps);

        UpdateWin(win);
        WaitVsync();
    }
}

#include "df_bitmap.h"
#include "df_font.h"
#include "df_time.h"
#include "df_window.h"
#include "fonts/df_mono.h"

#include <math.h>


// Based on https://physics.weber.edu/schroeder/fluids/

struct Cell {
    float n0;
    float nN, nS, nE, nW;
    float nNE, nSE, nNW, nSW;
    float rho, ux, uy, curl;
    bool barrier;
};

enum { X_DIM = 200 };
enum { Y_DIM = 80 };

// Index into this 2D array using x + y*X_DIM, traversing rows first and then columns.
Cell g_cells[X_DIM * Y_DIM];

Cell *cell(int x, int y) {
    return &g_cells[x + y * X_DIM];
}

float fluid_speed = 0.1;
float viscosity = 0.02;
int pxPerSquare = 4; // width of cell in pixels
float four9ths = 4.0 / 9.0;					// abbreviations
float one9th = 1.0 / 9.0;
float one36th = 1.0 / 36.0;
bool tracers_enabled = false;
enum { NUM_COLS = 400 };							// there are actually NUM_COLS+2 colors
DfColour colourList[NUM_COLS + 2];


// Set all densities in a cell to their equilibrium values for a given velocity and density:
void setEquil(int x, int y, float new_ux, float new_uy, float new_rho) {
    float ux3 = 3 * new_ux;
    float uy3 = 3 * new_uy;
    float ux2 = new_ux * new_ux;
    float uy2 = new_uy * new_uy;
    float uxuy2 = 2 * new_ux * new_uy;
    float u2 = ux2 + uy2;
    float u215 = 1.5 * u2;
    Cell *c = cell(x, y);
    c->n0 = four9ths * new_rho * (1 - u215);
    c->nE = one9th * new_rho * (1 + ux3 + 4.5 * ux2 - u215);
    c->nW = one9th * new_rho * (1 - ux3 + 4.5 * ux2 - u215);
    c->nN = one9th * new_rho * (1 + uy3 + 4.5 * uy2 - u215);
    c->nS = one9th * new_rho * (1 - uy3 + 4.5 * uy2 - u215);
    c->nNE = one36th * new_rho * (1 + ux3 + uy3 + 4.5 * (u2 + uxuy2) - u215);
    c->nSE = one36th * new_rho * (1 + ux3 - uy3 + 4.5 * (u2 - uxuy2) - u215);
    c->nNW = one36th * new_rho * (1 - ux3 + uy3 + 4.5 * (u2 - uxuy2) - u215);
    c->nSW = one36th * new_rho * (1 - ux3 - uy3 + 4.5 * (u2 + uxuy2) - u215);
    c->rho = new_rho;
    c->ux = new_ux;
    c->uy = new_uy;
}

// Set the fluid variables at the boundaries, according to the current slider value:
void setBoundaries() {
    float u0 = fluid_speed;
    for (int x = 0; x < X_DIM; x++) {
        setEquil(x, 0, u0, 0, 1);
        setEquil(x, Y_DIM - 1, u0, 0, 1);
    }
    for (int y = 1; y < Y_DIM - 1; y++) {
        setEquil(0, y, u0, 0, 1);
        setEquil(X_DIM - 1, y, u0, 0, 1);
    }
}

// Collide particles within each cell (here's the physics!):
void collide() {
    float viscosity = 0.02;	// kinematic viscosity coefficient in natural units
    float omega = 1.0 / (3.0 * viscosity + 0.5);		// reciprocal of relaxation time
    for (int y = 1; y < Y_DIM - 1; y++) {
        for (int x = 1; x < X_DIM - 1; x++) {
            Cell *c = cell(x, y);
            float this_rho = c->n0 + c->nN + c->nS + c->nE + c->nW + c->nNW + c->nNE + c->nSW + c->nSE;
            c->rho = this_rho;
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

// Move particles along their directions of motion:
void stream() {
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

// Move the tracer particles:
void moveTracers() {
//     for (int t = 0; t < NUM_TRACERS; t++) {
//         int roundedX = tracerX[t] + 0.5;
//         int roundedY = tracerY[t] + 0.5;
//         int index = roundedX + roundedY * X_DIM;
//         tracerX[t] += g_cells[index].ux;
//         tracerY[t] += g_cells[index].uy;
//         if (tracerX[t] > X_DIM - 1) {
//             tracerX[t] = 0.0;
//             tracerY[t] = (float)rand() / (float)RAND_MAX * Y_DIM;
//         }
//     }
}

void initTracers() {
//     if (tracers_enabled) {
//         int nRows = ceilf(sqrtf(NUM_TRACERS));
//         int dx = X_DIM / nRows;
//         int dy = Y_DIM / nRows;
//         int nextX = dx / 2;
//         int nextY = dy / 2;
//         for (int t = 0; t < NUM_TRACERS; t++) {
//             tracerX[t] = nextX;
//             tracerY[t] = nextY;
//             nextX += dx;
//             if (nextX > X_DIM) {
//                 nextX = dx / 2;
//                 nextY += dy;
//             }
//         }
//     }
}

// Compute the curl (actually times 2) of the macroscopic velocity field, for plotting:
void computeCurl() {
    for (int y = 1; y < Y_DIM - 1; y++) {			// interior sites only; leave edges set to zero
        for (int x = 1; x < X_DIM - 1; x++) {
            cell(x, y)->curl = cell(x + 1, y)->uy -
                cell(x - 1, y)->uy -
                cell(x, y + 1)->ux +
                cell(x, y - 1)->ux;
        }
    }
}

// Draw the tracer particles:
// void drawTracers() {
//     for (int t = 0; t < NUM_TRACERS; t++) {
//         int canvasX = (tracerX[t] + 0.5) * pxPerSquare;
//         int canvasY = canvas.height - (tracerY[t] + 0.5) * pxPerSquare;
//         context.fillRect(canvasX - 1, canvasY - 1, 2, 2);
//     }
// }

// Initialize or re-initialize the fluid, based on speed slider setting:
void initFluid() {
    float u0 = fluid_speed;
    for (int y = 0; y < Y_DIM; y++) {
        for (int x = 0; x < X_DIM; x++) {
            setEquil(x, y, u0, 0, 1);
            cell(x, y)->curl = 0.0;
        }
    }
}

void paintCanvas() {
    int cIndex = 0;
    float contrast = 1.0;
    int plotType = 4;
    if (plotType == 4) computeCurl();
    for (int y = 0; y < Y_DIM; y++) {
        for (int x = 0; x < X_DIM; x++) {
            if (cell(x, y)->barrier) {
                cIndex = NUM_COLS + 1;	// kludge for barrier color which isn't really part of color map
            }
            else {
                if (plotType == 0) {
                    cIndex = NUM_COLS * ((cell(x, y)->rho - 1) * 6 * contrast + 0.5) + 0.5;
                }
                else if (plotType == 1) {
                    cIndex = (NUM_COLS * (cell(x, y)->ux * 2 * contrast + 0.5)) + 0.5;
                }
                else if (plotType == 2) {
                    cIndex = (NUM_COLS * (cell(x, y)->uy * 2 * contrast + 0.5)) + 0.5;
                }
                else if (plotType == 3) {
                    float speed = sqrtf(cell(x, y)->ux * cell(x, y)->ux + cell(x, y)->uy * cell(x, y)->uy);
                    cIndex = (NUM_COLS * (speed * 4 * contrast)) + 0.5;
                }
                else {
                    cIndex = (NUM_COLS * (cell(x, y)->curl * 5 * contrast + 0.5)) + 0.5;
                }
                if (cIndex < 0) cIndex = 0;
                if (cIndex > NUM_COLS) cIndex = NUM_COLS;
            }
            RectFill(g_window->bmp, x * pxPerSquare, y * pxPerSquare, 
                pxPerSquare, pxPerSquare, colourList[cIndex]);
        }
    }

    // Draw tracers, force vector, and/or sensor if appropriate:
    //    if (tracers_enabled) drawTracers();
}

// Simulate void executes a bunch of steps and then schedules another call to itself:
void simulate() {
    setBoundaries();

    // Execute a bunch of time steps:
    int stepsPerFrame = 20;			// number of simulation steps per animation frame
    for (int step = 0; step < stepsPerFrame; step++) {
        collide();
        stream();
        if (tracers_enabled) moveTracers();
    }
    paintCanvas();

    //     int stable = true;
    //     for (int x = 0; x < X_DIM; x++) {
    //         int index = x + (Y_DIM / 2) * X_DIM;	// look at middle row only
    //         if (g_cells[index].rho <= 0) stable = false;
    //     }
    //     if (!stable) {
    //         //window.alert("The simulation has become unstable due to excessive fluid speeds.");
    //         initFluid();
    //     }
}

void LatticeBoltzmannMain()
{
    CreateWin(800, 320, WT_WINDOWED, "Lattice Boltzmann Method Fluid Simulation Example");
    g_defaultFont = LoadFontFromMemory(deadfrog_mono_7x13, sizeof(deadfrog_mono_7x13));

    // Initialize to no barriers.
    for (int y = 0; y < Y_DIM; y++) {
        for (int x = 0; x < X_DIM; x++) {
            cell(x, y)->barrier = false;
        }
    }

    // Create a simple linear "wall" barrier (intentionally a little offset from center):
    int barrier_size = 8;
    for (int y = (Y_DIM / 2) - barrier_size; y <= (Y_DIM / 2) + barrier_size; y++) {
        int x = Y_DIM / 3.0 + 0.5;
        cell(x, y)->barrier = true;
    }

    // Set up the array of colors for plotting (mimicks matplotlib's "jet" colormap):
    // (Kludge: Index NUM_COLS+1 labels the color used for drawing barriers.)
    for (int c = 0; c <= NUM_COLS; c++) {
        int r, g, b;
        if (c < NUM_COLS / 8) {
            r = 0; g = 0; b = 255.0 * (c + NUM_COLS / 8.0) / (NUM_COLS / 4.0);
        }
        else if (c < 3 * NUM_COLS / 8.0) {
            r = 0; g = 255 * (c - NUM_COLS / 8.0) / (NUM_COLS / 4.0); b = 255;
        }
        else if (c < 5 * NUM_COLS / 8.0) {
            r = 255 * (c - 3 * NUM_COLS / 8.0) / (NUM_COLS / 4.0); g = 255; b = 255 - r;
        }
        else if (c < 7 * NUM_COLS / 8.0) {
            r = 255; g = 255 * (7 * NUM_COLS / 8.0 - c) / (NUM_COLS / 4.0); b = 0;
        }
        else {
            r = 255 * (9 * NUM_COLS / 8.0 - c) / (NUM_COLS / 4.0); g = 0; b = 0;
        }
        colourList[c] = Colour(r, g, b);
    }
    colourList[NUM_COLS + 1] = Colour(0, 0, 0);

    // Initialize tracers (but don't place them yet):
    enum { NUM_TRACERS = 144 };
    int tracerX[NUM_TRACERS];
    int tracerY[NUM_TRACERS];
    for (int t = 0; t < NUM_TRACERS; t++) {
        tracerX[t] = 0.0; tracerY[t] = 0.0;
    }

    initFluid();		// initialize to steady rightward flow

    while (!g_window->windowClosed && !g_input.keys[KEY_ESC])
    {
        InputPoll();

        simulate();

        // Draw frames per second counter
        RectFill(g_window->bmp, g_window->bmp->width - 55, 0, 55, g_defaultFont->charHeight + 2, g_colourBlack);
        DrawTextLeft(g_defaultFont, g_colourWhite, g_window->bmp, g_window->bmp->width - 50, 0, "FPS:%i", g_window->fps);

        UpdateWin();
//        WaitVsync();
    }
}

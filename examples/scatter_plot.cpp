// Very simple code to draw a scatter plot.
// Takes a CSV file as input.
// CSV format:
// * One row per X value.
// * First item on row is X co-ord.
// * Remaining items are Y co-ord(s) for one or more series.
// * Each row must have the same number of items.
// You will have to edit the code to define the range of the axes etc.

#include "df_bitmap.h"
#include "df_font.h"
#include "df_polygon_aa.h"
#include "df_window.h"
#include "fonts/df_mono.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


static float *g_series[10];
static int g_numSeries;

static void FatLineAa(DfBitmap *bmp, float x1, float y1, float x2, float y2, float width, DfColour col) {
    x1 *= 16.0f;
    y1 *= 16.0f;
    x2 *= 16.0f;
    y2 *= 16.0f;
    float dx = x2 - x1;
    float dy = y2 - y1;
    float invLen = width / sqrtf(dx * dx + dy * dy);
    float normX = -dy * invLen;
    float normY = dx * invLen;

    DfVec2 verts[4] = {
        { x1 + normX, y1 + normY },
        { x2 + normX, y2 + normY },
        { x2 - normX, y2 - normY },
        { x1 - normX, y1 - normY }
    };
    FillPolygonAa(bmp, verts, 4, col);
}

void ScatterPlotMain() {
    DfWindow *win = CreateWin(1600, 1200, WT_WINDOWED_RESIZEABLE, "Scatter Plot Example");
    g_defaultFont = LoadFontFromMemory(df_mono_8x15, sizeof(df_mono_8x15));

    DfBitmap *bmp = win->bmp;

    FILE *in = fopen("../../scatter_plot.csv", "r");
    if (!in) 
        return;

    // Count number of columns
    char line[80];
    if (!fgets(line, sizeof(line), in))
        return;
    g_numSeries = 1;
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == ',' || line[i] == ' ') {
            g_numSeries++;
        }
    }

    // Count number of rows
    int numRows = 1;
    while (fgets(line, sizeof(line), in))
        numRows++;
    fseek(in, 0, SEEK_SET);

    // Allocate space for data
    for (int i = 0; i < g_numSeries; i++)
        g_series[i] = new float[numRows];

    // Read data
    double minX = 0, maxX = 0;
    double minY = 0, maxY = 50;
    for (int i = 0; i < numRows; i++) {
        fgets(line, sizeof(line), in);
        
        char *nextToken;
        double val = strtod(line, &nextToken);
        g_series[0][i] = val;
        if (val < minX) minX = val;
        if (val > maxX) maxX = val;
        for (int j = 1; j < g_numSeries; j++) {
            nextToken++; // Skip comma or space
            val = strtod(nextToken, &nextToken);
            if (val < minY) minY = val;
            if (val > maxY) maxY = val;
            g_series[j][i] = val;
        }
    }

    int const border = 100;
    int const chartPosX = border;
    int const chartPosY = win->bmp->height - border;
    int const chartWidth = win->bmp->width - border * 2;
    int const chartHeight = win->bmp->height - border * 2;
    float scaleX = (float)chartWidth / maxX;
    float scaleY = (float)chartHeight / maxY;

    DfColour seriesColours[10] = {
        0xffdd6600, // Orange
        0xff0077cc, // Blueish
        0xffaa00aa, // Magenta
        0xff000000, // Black
    };

    float labelPosX = chartPosY + 20.0f;
    float labelPosY = chartPosX - 20.0f;
    float tickLen = 8.0f;
    float tickIntervalX = 5.0f;
    float labelIntervalX = 10.0f;
    float tickIntervalY = 1.0f;
    float labelIntervalY = 5.0f;

    while (!win->windowClosed && !win->input.keys[KEY_ESC]) {
        WaitVsync();
        if (!InputPoll(win))
            continue;

        BitmapClear(bmp, g_colourWhite);

        // Draw Legend
        if (0) {
            int legendPosX = chartPosX + chartWidth / 2;
            int legendPosY = border * 2;

            DrawTextSimple(g_defaultFont, g_colourBlack, bmp, legendPosX, legendPosY - g_defaultFont->charHeight/2, "tud_task()");
            FatLineAa(bmp, legendPosX - 25, legendPosY, legendPosX - 5, legendPosY, 5, seriesColours[1]);
            
            legendPosY += g_defaultFont->charHeight * 2;
            DrawTextSimple(g_defaultFont, g_colourBlack, bmp, legendPosX, legendPosY - g_defaultFont->charHeight / 2, "tud_audio_read()");
            FatLineAa(bmp, legendPosX - 25, legendPosY, legendPosX - 5, legendPosY, 5, seriesColours[2]);
        }

        // Draw X axis
        {
            FatLineAa(bmp, chartPosX, chartPosY, chartPosX + chartWidth, chartPosY, 8, g_colourBlack);
            float endVal = g_series[0][numRows - 1];
            for (float val = 0.0f; val < endVal; val += tickIntervalX) {
                float x = chartPosX + val * scaleX;
                FatLineAa(bmp, x, chartPosY, x, chartPosY + tickLen, 4, g_colourBlack);
            }
            for (float val = 0.0f; val < endVal; val += labelIntervalX) {
                float x = chartPosX + val * scaleX;
                DrawTextCentre(g_defaultFont, g_colourBlack, bmp, x, 
                    chartPosY + g_defaultFont->charHeight, "%.0f", val);
            }

            DrawTextRight(g_defaultFont, g_colourBlack, bmp, chartPosX + chartWidth, 
                chartPosY + g_defaultFont->charHeight * 2, "Time in milliseconds");
        }

        // Draw Y axis
        {
            FatLineAa(bmp, chartPosX, chartPosY, chartPosX, chartPosY - chartHeight, 8, g_colourBlack);
            float endVal = maxY;
            for (float val = 0.0f; val < endVal; val += tickIntervalY) {
                float y = chartPosY - val * scaleY;
                FatLineAa(bmp, chartPosX - tickLen, y, chartPosX, y, 4, g_colourBlack);
            }
            for (float val = 0.0f; val < endVal; val += labelIntervalY) {
                float y = chartPosY - val * scaleY;
                DrawTextRight(g_defaultFont, g_colourBlack, bmp, labelPosY, y - g_defaultFont->charHeight / 2, "%.0f", val);
            }

            DrawTextCentre(g_defaultFont, g_colourBlack, bmp, chartPosX, border - g_defaultFont->charHeight * 2, "Time in microseconds");
        }

        for (int j = 1; j < g_numSeries; j++) {
            float *yVals = g_series[j];
            int prevY = chartPosY - yVals[0] * scaleY;
            for (int i = 1; i < numRows; i++) {
                float x1 = chartPosX + g_series[0][i - 1] * scaleX;
                float x2 = chartPosX + g_series[0][i] * scaleX;
                float y = chartPosY - yVals[i] * scaleY;
                FatLineAa(bmp, x1, prevY, x2, y, 5, seriesColours[j]);
                prevY = y;
            }
        }

        UpdateWin(win);
    }
}

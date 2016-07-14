#include "df_time.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


static double g_tickInterval = -1.0;
static double g_timeShift = 0.0;


inline double GetLowLevelTime();


static void InitialiseHighResTime()
{
    LARGE_INTEGER count;
    QueryPerformanceFrequency(&count);
    g_tickInterval = 1.0 / (double)count.QuadPart;
	g_timeShift = GetLowLevelTime();
}


inline double GetLowLevelTime()
{
    if (g_tickInterval < 0.0)
        InitialiseHighResTime();

    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * g_tickInterval;
}


NOINLINE double DfGetTime()
{
	double timeNow = GetLowLevelTime();
    timeNow -= g_timeShift;
    return timeNow;
}


void DfSleepMillisec(int milliseconds)
{
	Sleep(milliseconds);
}

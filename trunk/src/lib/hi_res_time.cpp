#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <math.h>

#include "hi_res_time.h"


static double g_tickInterval = -1.0;
static double g_lastGetHighResTime = 0.0;
static double g_timeShift = 0.0;


inline double GetLowLevelTime();


void InitialiseHighResTime()
{
	// Start be getting the frequency the Performance Counter uses.
	// We need to use the Performance Counter to calibrate the Pentium
	// Counter, if we are going to use it.
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


double GetHighResTime()
{
	double timeNow = GetLowLevelTime();
    timeNow -= g_timeShift;

    if( timeNow > g_lastGetHighResTime + 1.0f )
    {
        g_timeShift += timeNow - g_lastGetHighResTime;
        timeNow -= timeNow - g_lastGetHighResTime;
    }

    g_lastGetHighResTime = timeNow;
    return timeNow;
}


void SleepMillisec(int milliseconds)
{
	Sleep(milliseconds);
}

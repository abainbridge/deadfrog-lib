#include "df_time.h"


static double g_timeShift = 0.0;


#if _MSC_VER


// Windows specific code ******************************************************

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


static double g_tickInterval = -1.0;


inline double GetLowLevelTime()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * g_tickInterval;
}


static void InitialiseHighResTime()
{
    LARGE_INTEGER count;
    QueryPerformanceFrequency(&count);
    g_tickInterval = 1.0 / (double)count.QuadPart;
	g_timeShift = GetLowLevelTime();
}


void SleepMillisec(int milliseconds)
{
	Sleep(milliseconds);
}


#else


// Linux specific code ******************************************************

// POSIX headers
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>


static double GetLowLevelTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)((int64_t)tv.tv_sec * 1000000 + tv.tv_usec) / 1000000.;
}


static void InitialiseHighResTime()
{
	g_timeShift = GetLowLevelTime();
}


void SleepMillisec(int milliseconds)
{
	usleep(milliseconds * 1000);
}


#endif


// Common code ******************************************************

NOINLINE double GetRealTime()
{
    if (g_timeShift <= 0.0) {
        InitialiseHighResTime();
    }

    double timeNow = GetLowLevelTime();
    timeNow -= g_timeShift;
    return timeNow;
}

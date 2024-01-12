#pragma once


#ifndef __cplusplus
#include <stdbool.h>
#endif


#ifdef __cplusplus
extern "C"
{
#endif


#ifdef DLL_EXPORTS
 #define DLL_API extern "C" __declspec(dllexport)
#elif DLL_IMPORTS
 #define DLL_API extern "C" __declspec(dllimport)
#else
 #define DLL_API extern
#endif


#if _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif


void DebugOut(char const *fmt, ...);

void ReleaseAssert(bool condition, char const *fmt, ...);
void ReleaseWarn(bool condition, char const *fmt, ...);


#ifdef _MSC_VER
    #pragma warning(disable: 4244 4996)
    #define strcasecmp stricmp
    #define strncasecmp strnicmp
    #define __func__ __FUNCTION__
    #ifndef __cplusplus
        #define inline __inline
    #endif
#endif


static inline int IntMin(int a, int b) { return (a < b) ? a : b; };
static inline int IntMax(int a, int b) { return (a > b) ? a : b; };
static inline int RoundToInt(double r) {
    int tmp = (int)(r);
    tmp += (r-tmp>=.5) - (r-tmp<=-.5);
    return tmp;
}


static inline int ClampInt(int val, int min, int max) {
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}


static inline double ClampDouble(double val, double min, double max) {
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}


// Define an assert macro that stops in the debugger in Debug builds but does 
// nothing in release builds
#ifdef _DEBUG
 #ifdef _MSC_VER
  #include <crtdbg.h>
  #define DebugAssert(x) if(!(x)) { _CrtDbgBreak(); }
 #else
  #include <assert.h>
  #define DebugAssert(x) assert(x)
 #endif
#else
 #define DebugAssert(x)
#endif


#ifdef __cplusplus
}
#endif

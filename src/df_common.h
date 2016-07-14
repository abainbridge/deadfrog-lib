#pragma once


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

void DebugOut(char *fmt, ...);

void ReleaseAssert(int condition, char const *fmt, ...);
void ReleaseWarn(int condition, char const *fmt, ...);


#if (_MSC_VER < 1800)
int snprintf(char *s, size_t n, char const *fmt, ...);
#endif


inline int IntMin(int a, int b) { return (a < b) ? a : b; };
inline int IntMax(int a, int b) { return (a > b) ? a : b; };
inline int RoundToInt(double r) {
    int tmp = (int)(r);
    tmp += (r-tmp>=.5) - (r-tmp<=-.5);
    return tmp;
}


inline int ClampInt(int val, int min, int max)
{
    if (val < min)
        return min;
    if (val > max)
        return max;
    return val;
}


inline double ClampDouble(double val, double min, double max)
{
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

#ifndef INCLUDED_COMMON_INCLUDE_H
#define INCLUDED_COMMON_INCLUDE_H

#ifdef DLL_EXPORTS
 #define DLL_API extern "C" __declspec(dllexport)
#elif DLL_IMPORTS
 #define DLL_API extern "C" __declspec(dllimport)
#else
 #define DLL_API extern
#endif


void DebugOut(char *fmt, ...);

void ReleaseAssert(bool condition, char const *fmt, ...);
void ReleaseWarn(bool condition, char const *fmt, ...);


#if (_MSC_VER < 1800)
int snprintf(char *s, size_t n, char const *fmt, ...);
#endif


inline int IntMin(int a, int b) { return (a < b) ? a : b; };
inline int IntMax(int a, int b) { return (a > b) ? a : b; };
inline int RoundToInt(double r) {
    int tmp = static_cast<int> (r);
    tmp += (r-tmp>=.5) - (r-tmp<=-.5);
    return tmp;
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


#endif

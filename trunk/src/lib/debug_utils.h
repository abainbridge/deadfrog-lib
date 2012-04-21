#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H


void DebugOut(char *fmt, ...);


void ReleaseAssert(bool condition, char const *_fmt, ...);
void ReleaseWarn(bool condition, char const *_fmt, ...);

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

#ifndef INCLUDED_COMMON_INCLUDE_H
#define INCLUDED_COMMON_INCLUDE_H

#ifdef DLL_EXPORTS
 #define DLL_API extern "C" __declspec(dllexport)
#elif STATIC_LIB
 #define DLL_API extern
#else
 #define DLL_API extern "C" __declspec(dllimport)
#endif


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

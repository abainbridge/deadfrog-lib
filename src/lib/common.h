#ifndef INCLUDED_COMMON_INCLUDE_H
#define INCLUDED_COMMON_INCLUDE_H

#ifdef DLL_EXPORTS
 #define DLL_API extern "C" __declspec(dllexport)
#else
 #define DLL_API extern "C" __declspec(dllimport)
#endif

#endif

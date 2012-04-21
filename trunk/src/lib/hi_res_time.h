#ifndef HI_RES_TIME_H
#define HI_RES_TIME_H

#include "lib/common.h"

DLL_API void InitialiseHighResTime();
DLL_API double GetHighResTime();        // Return value in seconds
DLL_API void SleepMillisec(int milliseconds);

#endif


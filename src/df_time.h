#pragma once


#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


DLL_API double GetRealTime(); // Returns seconds since application start time.
DLL_API void SleepMillisec(int milliseconds);


#ifdef __cplusplus
}
#endif

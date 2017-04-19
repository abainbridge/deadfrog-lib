#pragma once


#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


DLL_API double DfGetTime();        // Return value in seconds
DLL_API void DfSleepMillisec(int milliseconds);


#ifdef __cplusplus
}
#endif

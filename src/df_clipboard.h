#pragma once


// TODO - Add a clipboard example.


#include "df_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


// Returns NULL if no data is present in the clipboard.
// Call ClipboardReleaseReceivedData() when done with
// the returned pointer (not needed if it was NULL).
DLL_API char *ClipboardReceiveData(int *numChars);

DLL_API void ClipboardReleaseReceivedData(char const *data);

// Returns 0 on failure, 1 on success.
DLL_API int ClipboardSetData(char const *data, int numChars);


#ifdef __cplusplus
}
#endif

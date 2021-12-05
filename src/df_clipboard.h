#pragma once


// TODO - Add a clipboard example.


#ifdef __cplusplus
extern "C"
{
#endif


// Returns NULL if no data is present in the clipboard.
// Call ClipboardReleaseReceivedData() when done with
// the returned pointer (not needed if it was NULL).
char *ClipboardReceiveData(int *numChars);

void ClipboardReleaseReceivedData(char const *data);

// Returns 0 on failure, 1 on success.
int ClipboardSetData(char const *data, int numChars);


#ifdef __cplusplus
}
#endif

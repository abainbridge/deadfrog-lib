#include "df_clipboard.h"

#include "df_common.h"

#include <stdlib.h>
#include <string.h>


#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


HANDLE g_clipboardData;


char *ClipboardReceiveData(int *numChars) {
    if (g_clipboardData) {
        DebugAssert("Clipboard already open");
        return NULL;
    }

    if (!OpenClipboard(NULL))
        return NULL;

    // Retrieve the Clipboard data (specifying that we want ANSI text.
    g_clipboardData = GetClipboardData(CF_TEXT);
    if (!g_clipboardData)
        return NULL;

    // Get a pointer to the data associated with the handle returned from
    // GetClipboardData.
    char *data = (char*)GlobalLock(g_clipboardData);
    *numChars = strlen(data) + 1;

    return data;
}


void ClipboardReleaseReceivedData(char const *data) {
    // Unlock the global memory.
    if (g_clipboardData)
        GlobalUnlock(g_clipboardData);
    g_clipboardData = NULL;

    // Close the Clipboard, which unlocks it so that other applications can
    // examine or modify its contents.
    CloseClipboard();
}


int ClipboardSetData(char const *data, int sizeData) {
    if (g_clipboardData) {
        DebugAssert(0); // Clipboard already open
        return 0;
    }

    if (!OpenClipboard(NULL))
        return 0;

    // Empty the Clipboard. This also has the effect of allowing Windows to
    // free the memory associated with any data that is in the Clipboard.
    EmptyClipboard();

    HGLOBAL clipboardHandle = GlobalAlloc(GMEM_DDESHARE, sizeData + 1);

    // Calling GlobalLock returns a pointer to the data associated with the
    // handle returned from GlobalAlloc()
    char *windowsData = (char*)GlobalLock(clipboardHandle);
    if (!windowsData)
        return 0;

    // Copy the data from the local variable to the global memory.
    memcpy(windowsData, data, sizeData);
    windowsData[sizeData] = '\0';

    // Unlock the memory - don't call GlobalFree because Windows will free the
    // memory automatically when EmptyClipboard is next called.
    GlobalUnlock(clipboardHandle);

    // Set the Clipboard data by specifying that ANSI text is being used and
    // passing the handle to the global memory.
    SetClipboardData(CF_TEXT, clipboardHandle);

    // Close the Clipboard which unlocks it so that other applications can
    // examine or modify its contents.
    CloseClipboard();

    return 1;
}


#else

#include <stdlib.h>


char *X11InternalClipboardRequestData();
void X11InternalClipboardReleaseReceivedData();
void X11InternalClipboardSetData(char const *data, int numChars);


char *ClipboardReceiveData(int *numChars) {
    return X11InternalClipboardRequestData();
}


void ClipboardReleaseReceivedData(char const *data) {
    X11InternalClipboardReleaseReceivedData();
}


int ClipboardSetData(char const *data, int numChars) {
    X11InternalClipboardSetData(data, numChars);
    return 0;
}


#endif

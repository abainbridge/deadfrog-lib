#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "lib/message_dialog.h"
#include "debug_utils.h"
#include "window_manager.h"


void DebugOut(char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
    OutputDebugString(buf);
}


void ReleaseAssert(bool condition, char const *_fmt, ...)
{
    static bool noAlready = false;
	if (!condition && !noAlready)
	{
		char buf[1024];
		va_list ap;
		va_start (ap, _fmt);
		vsprintf(buf, _fmt, ap);
		strcat(buf, "\nDo you want to quit?");
        int result = MessageDialog("Serious Error", buf, MsgDlgTypeYesNo);
#if defined(_DEBUG) && defined(_MSC_VER)
		result = result;
		_CrtDbgBreak();
#else
        if (result == MsgDlgRtnCode_Yes)
		    exit(-1);
        else
            noAlready = true;
#endif
	}
}


void ReleaseWarn(bool condition, char const *_fmt, ...)
{
	if (!condition)
	{
		char buf[512];
		va_list ap;
		va_start (ap, _fmt);
		vsprintf(buf, _fmt, ap);
		MessageDialog("Warning", buf, MsgDlgTypeOk);
	#if defined(_DEBUG) && defined(_MSC_VER)
		_CrtDbgBreak();
	#endif
	}
}

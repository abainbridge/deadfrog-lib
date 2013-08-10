#include "common.h"

#include "lib/message_dialog.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


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
	if (!condition)
	{
		char buf[1024];
		va_list ap;
		va_start (ap, _fmt);
		vsprintf(buf, _fmt, ap);
        MessageDialog("Serious Error", buf, MsgDlgTypeOk);
#if defined(_DEBUG) && defined(_MSC_VER)
		_CrtDbgBreak();
#else
	    exit(-1);
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

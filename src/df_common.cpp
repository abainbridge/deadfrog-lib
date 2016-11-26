#include "df_common.h"

#include "df_message_dialog.h"

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


#if (defined _MSC_VER && _MSC_VER < 1800)
int c99_vsnprintf(char *str, size_t size, char const *fmt, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, fmt, ap);
    if (count == -1)
        count = _vscprintf(fmt, ap);

    return count;
}


int snprintf(char *s, size_t n, char const *fmt, ...)
{
    int count;
    va_list ap;

    va_start(ap, fmt);
    count = c99_vsnprintf(s, n, fmt, ap);
    va_end(ap);

    return count;
}
#endif

#include "df_common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void DebugOut(char *_fmt, ...)
{
    va_list ap;
    va_start (ap, _fmt);
    vprintf(_fmt, ap);
}


void ReleaseAssert(bool condition, char const *_fmt, ...)
{
	if (!condition)
	{
		va_list ap;
		va_start (ap, _fmt);
		vprintf(_fmt, ap);
	    exit(-1);
	}
}


void ReleaseWarn(bool condition, char const *_fmt, ...)
{
	if (!condition)
	{
		va_list ap;
		va_start (ap, _fmt);
		vprintf(_fmt, ap);
	}
}
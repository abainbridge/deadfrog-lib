#include "df_common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void DebugOut(char *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    vprintf(fmt, ap);
}


void ReleaseAssert(bool condition, char const *fmt, ...)
{
	if (!condition)
	{
		va_list ap;
		va_start (ap, fmt);
		vprintf(fmt, ap);
	    exit(-1);
	}
}


void ReleaseWarn(bool condition, char const *fmt, ...)
{
	if (!condition)
	{
		va_list ap;
		va_start (ap, fmt);
		vprintf(fmt, ap);
	}
}

#include "df_common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void DebugOut(char const *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
}


void ReleaseAssert(bool condition, char const *fmt, ...) {
    if (!condition) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        putchar('\n');
        asm("int3");
        exit(-1);
    }
}


void ReleaseWarn(bool condition, char const *fmt, ...) {
    if (!condition) {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        putchar('\n');
    }
}

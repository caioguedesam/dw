#include "debug.hpp"
#include <stdio.h>

#ifndef _NOASSERT
void assert(uint64 expr, const char* msg)
{
    if(expr)
    {
        return;
    }

    // TODO_DW: MULTIPLATFORM
    MessageBoxExA(
            NULL,
            msg,
            "FAILED ASSERT",
            MB_OK,
            0);
    DebugBreak();
    ExitProcess(-1);
}

void assertf(uint64 expr, const char* fmt, ...)
{
    if(expr)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    char buf[2048];
    vsprintf(buf, fmt, args);

    // TODO_DW: MULTIPLATFORM
    MessageBoxExA(
            NULL,
            buf,
            "FAILED ASSERT",
            MB_OK,
            0);
    DebugBreak();
    ExitProcess(-1);
}
#endif

#ifndef _NOLOG
void logf(const char* label, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[2048];
    vsprintf(buf, fmt, args);
    printf("[%s]: %s\n", label, buf);
    va_end(args);
}
#endif


#pragma once
#include "../../PvzpLib/PvzpDebug.h"
#include <cstdarg>
#include <cstdio>

// Keep the pinned PC session's printf diagnostics without changing protocol code.
inline void NetworkLog(const char* format, ...)
{
    char message[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    PvzpLogLn("{}", message);
}

#pragma once
#include "base.hpp"

// Assert
#ifndef DW_DEBUG
#define ASSERT(EXPR)
#define ASSERTF(EXPR, ...)
#define STATIC_ASSERT(EXPR)
#define BREAK(EXPR)
#define BREAKF(EXPR, ...)
#else

void dwassert(uint64 expr, const char* msg);
void dwassertf(uint64 expr, const char* fmt, ...);
void dwbreak(uint64 expr, const char* msg);
void dwbreakf(uint64 expr, const char* fmt, ...);

#define ASSERT(EXPR) STMT(dwassert((uint64)(EXPR), STRINGIFY(EXPR)))
#define ASSERTF(EXPR, FMT, ...) STMT(dwassertf((uint64)(EXPR), FMT, __VA_ARGS__))
#define STATIC_ASSERT(EXPR) static_assert((EXPR))
#define BREAK(EXPR) STMT(dwbreak((uint64)(EXPR), STRINGIFY(EXPR)))
#define BREAKF(EXPR, FMT, ...) STMT(dwbreak((uint64)(EXPR), FMT, __VA_ARGS__))

#endif

// Log
#ifndef DW_DEBUG
#define LOG(MSG)
#define LOGL(LABEL, MSG)
#define LOGF(FMT, ...)
#define LOGLF(LABEL, FMT, ...)
#else

void logf(const char* label, const char* fmt, ...);

#define LOG(MSG) STMT(logf("LOG", "%s", MSG))
#define LOGL(LABEL, MSG) STMT(logf(LABEL, "%s", MSG))
#define LOGF(FMT, ...) STMT(logf("LOG", FMT, __VA_ARGS__))
#define LOGLF(LABEL, FMT, ...) STMT(logf(LABEL, FMT, __VA_ARGS__))

#endif

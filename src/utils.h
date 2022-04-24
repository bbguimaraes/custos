#ifndef CUSTOS_UTILS_H
#define CUSTOS_UTILS_H

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define LOG_ERR(...) log_err(__func__, __LINE__, __VA_ARGS__)
#define LOG_ERRNO(...) log_errno(__func__, __LINE__, __VA_ARGS__)

static inline void log_err(
    const char *restrict fn, int line, const char *restrict fmt, ...);
static inline void log_errno(
    const char *restrict fn, int line, const char *restrict fmt, ...);
static inline void vlog_err(
    const char *restrict fn, int line, const char *restrict fmt, va_list args);
static inline void vlog_errno(
    const char *restrict fn, int line, const char *restrict fmt, va_list args);

inline void log_err(
    const char *restrict fn, int line, const char *restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_err(fn, line, fmt, args);
    va_end(args);
}

inline void log_errno(
    const char *restrict fn, int line, const char *restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_errno(fn, line, fmt, args);
    va_end(args);
}

inline void vlog_err(
    const char *restrict fn, int line, const char *restrict fmt, va_list args)
{
    fprintf(stderr, "%s:%d: ", fn, line);
    vfprintf(stderr, fmt, args);
}

inline void vlog_errno(
    const char *restrict fn, int line, const char *restrict fmt, va_list args)
{
    vlog_err(fn, line, fmt, args);
    fprintf(stderr, ": %s\n", strerror(errno));
}

#endif

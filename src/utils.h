#ifndef CUSTOS_UTILS_H
#define CUSTOS_UTILS_H

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define LOG_ERR(...) log_err(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERRNO(...) log_errno(__FILE__, __LINE__, __func__, __VA_ARGS__)

static inline void log_err(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, ...);
static inline void log_errno(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, ...);
static inline void vlog_err(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, va_list args);
static inline void vlog_errno(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, va_list args);

inline void log_err(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_err(file, line, fn, fmt, args);
    va_end(args);
}

inline void log_errno(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vlog_errno(file, line, fn, fmt, args);
    va_end(args);
}

inline void vlog_err(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, va_list args)
{
    fprintf(stderr, "%s:%d: %s: ", file, line, fn);
    vfprintf(stderr, fmt, args);
}

inline void vlog_errno(
    const char *restrict file, int line, const char *restrict fn,
    const char *restrict fmt, va_list args)
{
    vlog_err(file, line, fn, fmt, args);
    fprintf(stderr, ": %s\n", strerror(errno));
}

#endif

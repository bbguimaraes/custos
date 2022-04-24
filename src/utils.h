#ifndef CUSTOS_UTILS_H
#define CUSTOS_UTILS_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

/**
 * Reads an unsigned integer value from a command-line argument.
 * \param name Argument name, displayed in error messages.
 * \param max Maximum return value allowed.
 * \param s The argument string.
 * \param p Output value.
 */
static inline bool parse_ulong_arg(
    const char *name, size_t max, const char *s, unsigned long *p);

/**
 * Finds a string in an array.
 * \param v Null-terminated array of null-terminated strings.
 * \param s The string, matched via `strcmp`.
 */
static inline char *find_str(const char *const *v, const char *s);

inline bool parse_ulong_arg(
    const char *name, size_t max, const char *s, unsigned long *p
) {
    const char *se = NULL;
    unsigned long u = 0;
    u = strtoul(s, (char**)&se, 10);
    if(!u && se != s) {
        fprintf(
            stderr, "%s: invalid unsigned integer argument \"%s\": %s\n",
            __func__, name, s);
        return false;
    }
    if(max < u) {
        fprintf(
            stderr, "%s: argument \"%s\" out of range: %zu < %lu\n",
            __func__, name, max, u);
        return false;
    }
    *p = u;
    return true;
}

inline char *find_str(const char *const *v, const char *s) {
    for(; *v; ++v)
        if(strcmp(*v, s) == 0)
            return (char*)s;
    return NULL;
}

#endif

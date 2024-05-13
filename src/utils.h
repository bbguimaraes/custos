#ifndef CUSTOS_UTILS_H
#define CUSTOS_UTILS_H

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#define CUSTOS_MAX_PATH 4096

#define MIN(x, y) ((y) < (x) ? (y) : (x))
#define MAX(x, y) ((y) > (x) ? (y) : (x))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))
#define LOG_ERR(...) log_err(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERRNO(...) log_errno(__FILE__, __LINE__, __func__, __VA_ARGS__)

typedef uint8_t u8;

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

static inline FILE *open_file(const char *path, const char *mode);
static inline bool close_file(FILE *f, const char *path);
static inline bool rewind_and_scan(FILE *f, const char *format, void *ret);
static inline bool rewind_and_read(
    FILE *f, const char *restrict name, size_t *n, char *restrict p);
bool join_path(char v[static CUSTOS_MAX_PATH], int n, ...);
bool file_exists(const char *name);

/**
 * Reads an unsigned integer value from a command-line argument.
 * \param name Argument name, displayed in error messages.
 * \param max Maximum return value allowed.
 * \param s The argument string.
 * \param p Output value.
 */
static inline bool parse_ulong_arg(
    const char *name, size_t max, const char *s, unsigned long *p);

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
            stderr, "%s: argument \"%s\" out of range: %lu < %lu\n",
            __func__, name, max, u);
        return false;
    }
    *p = u;
    return true;
}

inline FILE *open_file(const char *path, const char *mode) {
    FILE *const ret = fopen(path, mode);
    if(!ret)
        LOG_ERRNO("fopen(%s)", path);
    return ret;
}

inline bool close_file(FILE *f, const char *path) {
    if(fclose(f)) {
        LOG_ERRNO("fclose(%s)", path);
        return false;
    }
    return true;
}

inline bool rewind_and_scan(FILE *f, const char *format, void *p) {
    if(fflush(f) == EOF)
        return LOG_ERRNO("fflush"), false;
    rewind(f);
    if(fscanf(f, format, p) != 1) {
        LOG_ERRNO("invalid value read from file, error");
        return false;
    }
    return true;
}

inline bool rewind_and_read(
    FILE *f, const char *restrict name, size_t *n, char *restrict p)
{
    if(fflush(f) == EOF)
        return LOG_ERRNO("fflush"), false;
    rewind(f);
    const size_t r = fread(p, 1, *n, f);
    if(ferror(f))
        return LOG_ERRNO("fread"), false;
    if(r == *n)
        return LOG_ERR("%s too long: %.*s", name, *n, p), false;
    *n = r;
    return true;
}

#endif

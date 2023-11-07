#include "utils.h"

#include <assert.h>

static inline size_t strlcpy(
    char *restrict dst, const char *restrict src, size_t n)
{
    const char *const p = dst;
    while(n && (*dst++ = *src++))
        --n;
    return (size_t)(dst - p);
}

bool join_path(char v[static CUSTOS_MAX_PATH], int n, ...) {
    va_list args;
    va_start(args, n);
    char *p = v, *const e = v + CUSTOS_MAX_PATH;
    for(int i = 0; i != n; ++i) {
        const size_t ni =
            strlcpy(p, va_arg(args, const char*), (size_t)(e - p));
        if(p[ni - 1])
            goto err;
        p += ni - 1;
    }
    assert(p <= e);
    assert(!e[-1]);
    va_end(args);
    return true;
err:
    va_end(args);
    LOG_ERR("path too long: %s\n", v);
    return false;
}

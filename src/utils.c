#include "utils.h"

#include <assert.h>

#include "window.h"

static inline size_t strlcpy(
    char *restrict dst, const char *restrict src, size_t n)
{
    const char *const p = dst;
    while(n && (*dst++ = *src++))
        --n;
    return (size_t)(dst - p);
}

void print_perc(struct window *w, float f) {
    if(f == 100.0f)
        window_print(w, " 100%");
    else
        window_printf(w, "%4.1f%%", f);
}

void print_size(struct window *w, size_t n) {
    const char *suffix[] = {"", "K", "M", "G", "T"};
    int i = 0;
    size_t div = 1;
    while(1000 <= n / div && i < (int)ARRAY_SIZE(suffix))
        ++i, div *= 1024;
    const double d = (double)n / (double)div;
    if(10 <= n / div)
        window_printf(w, "%3d%s", (int)round(d), suffix[i]);
    else
        window_printf(w, "%3.1f%s", d, suffix[i]);
}

void print_bar(struct window *w, float v) {
    window_print(w, "[");
    const int WIDTH = 16;
    int n = (int)(v / 100.0f * (float)WIDTH);
    for(int i = 0; i != n; ++i)
        window_print(w, "=");
    n = WIDTH - n;
    for(int i = 0; i != n; ++i)
        window_print(w, "-");
    window_print(w, "]");
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

bool file_exists(const char *name) {
    FILE *const f = fopen(name, "r");
    if(!f)
        return false;
    fclose(f);
    return true;
}

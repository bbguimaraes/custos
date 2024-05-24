#include "window.h"

#include <stdarg.h>

#include "term.h"

bool window_screen_size(int *w, int *h) {
    return term_size(stdout, w, h);
}

bool window_init(struct window *w, int width, int height, int x, int y) {
    (void)w, (void)width, (void)height, (void)x, (void)y;
    return true;
}

void window_destroy(struct window *w) {
    (void)w;
}

void window_clear(struct window *w) {
    (void)w;
    term_clear(stdout);
}

void window_normal_text(struct window *w) {
    (void)w;
    term_normal_text(stdout);
}

void window_bold_text(struct window *w) {
    (void)w;
    term_bold_text(stdout);
}

void window_print(struct window *w, const char *s) {
    (void)w;
    fputs(s, stdout);
}

void window_printf(struct window *w, const char *fmt, ...) {
    (void)w;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void window_refresh(struct window *w) {
    (void)w;
}

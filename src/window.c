#include "window.h"

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

void window_refresh(struct window *w) {
    (void)w;
}

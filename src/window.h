#ifndef CUSTOS_WINDOW_H
#define CUSTOS_WINDOW_H

#include <stdbool.h>

struct window {
    struct WINDOW *w;
    int width, height, x, y;
};

bool window_screen_size(int *w, int *h);

bool window_init(struct window *w, int width, int height, int x, int y);
void window_destroy(struct window *w);
void window_clear(struct window *w);
void window_refresh(struct window *w);

#endif

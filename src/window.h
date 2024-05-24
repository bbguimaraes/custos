#ifndef CUSTOS_WINDOW_H
#define CUSTOS_WINDOW_H

#include <stdbool.h>

struct WINDOW;

struct window {
    struct WINDOW *w;
    int width, height, x, y;
};

void window_init_curses(void);
void window_destroy_curses(void);
bool window_input(void);

bool window_screen_size(int *w, int *h);

bool window_init(struct window *w, int width, int height, int x, int y);
void window_destroy(struct window *w);
void window_clear(struct window *w);
void window_normal_text(struct window *w);
void window_bold_text(struct window *w);
void window_print(struct window *w, const char *s);
void window_printf(struct window *w, const char *fmt, ...);
void window_refresh(struct window *w);

#endif

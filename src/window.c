#include "window.h"

#include <stdarg.h>

#include <curses.h>

#include "term.h"

#define W(x) ((WINDOW*)w->w)

static bool curses;

void window_init_curses(void) {
    curses = true;
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    timeout(0);
}

void window_destroy_curses(void) {
    curses = false;
    echo();
    noraw();
    endwin();
}

bool window_input(void) {
    for(;;)
        switch(getch()) {
        case ERR:
            return false;
        case KEY_RESIZE:
            break;
        default:
            return true;
        }
}

bool window_screen_size(int *w, int *h) {
    if(!curses)
        return term_size(stdout, w, h);
    *w = COLS;
    *h = LINES;
    return true;
}

bool window_init(struct window *w, int width, int height, int x, int y) {
    if(!curses)
        return true;
    WINDOW *const cw = newwin(height, width, y, x);
    if(!cw)
        return false;
    w->w = (struct WINDOW*)cw;
    return true;
}

void window_destroy(struct window *w) {
    (void)w;
}

void window_clear(struct window *w) {
    if(!curses)
        return;
    wclear(W(w));
}

void window_normal_text(struct window *w) {
    if(curses)
        wattr_off(W(w), A_BOLD, NULL);
    else
        term_normal_text(stdout);
}

void window_bold_text(struct window *w) {
    if(curses)
        wattr_on(W(w), A_BOLD, NULL);
    else
        term_bold_text(stdout);
}

void window_print(struct window *w, const char *s) {
    if(curses)
        wprintw(W(w), "%s", s);
    else
        fputs(s, stdout);
}

void window_printf(struct window *w, const char *fmt, ...) {
    (void)w;
    va_list args;
    va_start(args, fmt);
    if(curses)
        vw_printw(W(w), fmt, args);
    else
        vprintf(fmt, args);
    va_end(args);
}

void window_refresh(struct window *w) {
    if(!curses)
        return;
    wrefresh(W(w));
}

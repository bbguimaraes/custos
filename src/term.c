#include "term.h"

#include <sys/ioctl.h>

#include "utils.h"

#define ESC "\x1b"
#define CMD ESC "["
#define CLEAR CMD "H" CMD "J"
#define RESET CMD "m\x0f"
#define BOLD CMD "1m"

void term_clear(FILE *f) {
    fputs(CLEAR, f);
}

bool term_size(FILE *f, int *w, int *h) {
    struct winsize s;
    if(ioctl(fileno(f), TIOCSWINSZ, &s) == -1)
        return LOG_ERRNO("ioctl(TIOCSWINSZ)", 0), false;
    *w = s.ws_col;
    *h = s.ws_row;
    return true;
}

void term_normal_text(FILE *f) {
    fputs(RESET, f);
}

void term_bold_text(FILE *f) {
    fputs(BOLD, f);
}

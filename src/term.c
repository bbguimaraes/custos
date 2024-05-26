#include "term.h"

#define ESC "\x1b"
#define CMD ESC "["
#define CLEAR CMD "H" CMD "J"
#define RESET CMD "m\x0f"
#define BOLD CMD "1m"

void term_clear(FILE *f) {
    fputs(CLEAR, f);
}

void term_normal_text(FILE *f) {
    fputs(RESET, f);
}

void term_bold_text(FILE *f) {
    fputs(BOLD, f);
}

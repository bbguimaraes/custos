#include "term.h"

#define ESC "\x1b"
#define CMD ESC "["
#define CLEAR CMD "H" CMD "J"

void term_clear(FILE *f) {
    fputs(CLEAR, f);
}

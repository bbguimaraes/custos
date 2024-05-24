#ifndef CUSTOS_TERM_H
#define CUSTOS_TERM_H

#include <stdbool.h>
#include <stdio.h>

void term_clear(FILE *f);
bool term_size(FILE *f, int *w, int *h);
void term_normal_text(FILE *f);
void term_bold_text(FILE *f);

#endif

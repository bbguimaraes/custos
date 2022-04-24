#ifndef CUSTOS_LOAD_H
#define CUSTOS_LOAD_H

#include <stdbool.h>

void *load_init(void);
bool load_destroy(void *d);
bool load_update(void *d);

#endif

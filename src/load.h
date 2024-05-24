#ifndef CUSTOS_LOAD_H
#define CUSTOS_LOAD_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void *load_init(struct lua_State *L);
bool load_destroy(void *d);
bool load_update(void *d, size_t counter, struct window *w);

#endif

#ifndef CUSTOS_LOAD_H
#define CUSTOS_LOAD_H

#include <stdbool.h>

struct lua_State;

void *load_init(struct lua_State *L);
bool load_destroy(void *d);
bool load_update(void *d);

#endif

#ifndef CUSTOS_THERMAL_H
#define CUSTOS_THERMAL_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void thermal_lua(struct lua_State *L);
void *thermal_init(struct lua_State *L);
bool thermal_destroy(void *d);
bool thermal_update(void *d, size_t counter, struct window *w);

#endif

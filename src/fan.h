#ifndef CUSTOS_FAN_H
#define CUSTOS_FAN_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void fan_lua(struct lua_State *L);
void *fan_init(struct lua_State *L);
bool fan_destroy(void *d);
bool fan_update(void *d, size_t counter, struct window *w);

#endif

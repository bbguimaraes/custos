#ifndef CUSTOS_BATTERY_H
#define CUSTOS_BATTERY_H

#include <stdbool.h>

struct lua_State;

void *battery_init(struct lua_State *L);
bool battery_destroy(void *d);
bool battery_update(void *d);

#endif

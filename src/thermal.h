#ifndef CUSTOS_THERMAL_H
#define CUSTOS_THERMAL_H

#include <stdbool.h>

struct lua_State;

void *thermal_init(struct lua_State *L);
bool thermal_destroy(void *d);
bool thermal_update(void *d);

#endif

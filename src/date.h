#ifndef CUSTOS_DATE_H
#define CUSTOS_DATE_H

#include <stdbool.h>

struct lua_State;

void *date_init(struct lua_State *L);
bool date_destroy(void *d);
bool date_update(void *d);

#endif

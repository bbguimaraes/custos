#ifndef CUSTOS_DATE_H
#define CUSTOS_DATE_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void date_lua(struct lua_State *L);
void *date_init(struct lua_State *L);
bool date_destroy(void *d);
bool date_update(void *d, size_t counter, struct window *w);

#endif

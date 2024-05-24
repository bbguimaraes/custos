#ifndef CUSTOS_FS_H
#define CUSTOS_FS_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void fs_lua(struct lua_State *L);
void *fs_init(struct lua_State *L);
bool fs_destroy(void *d);
bool fs_update(void *d, size_t counter, struct window *w);

#endif

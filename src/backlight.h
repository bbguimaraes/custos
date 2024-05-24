#ifndef CUSTOS_BACKLIGHT_H
#define CUSTOS_BACKLIGHT_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void backlight_lua(struct lua_State *L);
void *backlight_init(struct lua_State *L);
bool backlight_destroy(void *d);
bool backlight_update(void *d, size_t counter, struct window *w);

#endif

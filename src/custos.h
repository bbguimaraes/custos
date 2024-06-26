#ifndef CUSTOS_H
#define CUSTOS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct lua_State;

struct window;

struct lua_State *custos_lua_init(void);
void custos_lua_destroy(struct lua_State *L);
bool custos_load_config(
    struct lua_State *L, uint8_t *enabled_modules, struct window **windows,
    size_t *n_windows, size_t ***modules);

#endif

#ifndef CUSTOS_GRAPH_H
#define CUSTOS_GRAPH_H

#include <stddef.h>

struct lua_State;
struct window;

struct graph {
    int w, h, i;
};

void graph_lua_read(struct graph *g, struct lua_State *L);
void graph_update(
    const struct graph *g, size_t n, void *p, const void *x, size_t size,
    void (*acc)(void*, const void*, size_t));
void graph_render(
    struct window *w, const struct graph *g, size_t max, void *p,
    float (*idx)(void*, int));

#endif

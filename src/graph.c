#include "graph.h"

#include <string.h>

#include <lua.h>

#include "utils.h"
#include "window.h"

void graph_lua_read(struct graph *g, struct lua_State *L) {
    if(lua_getfield(L, -2, "graph") == LUA_TNIL)
        return;
    if(lua_getfield(L, -1, "width") != LUA_TNIL)
        g->w = (int)lua_tointeger(L, -1);
    if(lua_getfield(L, -2, "height") != LUA_TNIL)
        g->h = (int)lua_tointeger(L, -1);
    lua_pop(L, 2);
}

void graph_update(
    const struct graph *g, size_t n, void *d, const void *x, size_t size,
    void (*acc)(void*, const void*, size_t)
) {
    const size_t i = (size_t)g->i;
    const size_t max = (size_t)g->w - 1;
    char *const p = (char*)d;
    if(!n && i == max) {
        memmove(p, p + 1, size * max);
        memset(p + i, 0, size);
    }
    acc(p + i, x, n);
}

void graph_render(
    struct window *w, const struct graph *g, size_t max, void *p,
    float (*idx)(void*, int))
{
    const int width = g->w, height = g->h, i = g->i;
    const char bars[][sizeof("█")] =
        {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const int n = ARRAY_SIZE(bars);
    const float max_f = nextafterf(1.0f, 0.0f);
    for(int y = height; y--;) {
        window_print(w, "\n   ");
        for(int x = 0; x <= i; ++x) {
            const float fv = idx(p, x) / (float)max * (float)height - (float)y;
            const float fi = floorf(CLAMP(fv, 0.0f, max_f) * (float)n);
            window_print(w, bars[(int)fi]);
        }
    }
    window_printf(w, "%*s", width - i - 1, "");
}

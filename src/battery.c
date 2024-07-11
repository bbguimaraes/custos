#include "battery.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>

#include "utils.h"
#include "window.h"

#define NOW "energy_now"
#define FULL "energy_full"
#define DESIGN "energy_full_design"
#define STATUS "status"
#define VAR_LUA "battery.batteries"

#define STATUS_MAX 32

#define GRAPH_WIDTH 24
#define GRAPH_UPDATE_RATE (15 * 60)

struct battery {
    char *name, *base;
    unsigned long *graph;
    FILE *now, *full, *design, *status;
};

struct data {
    int graph_i;
    struct battery v[];
};

static int set_batteries(lua_State *L) {
    lua_setglobal(L, VAR_LUA);
    return 0;
}

static struct data *get_batteries(lua_State *L) {
    if(lua_getglobal(L, VAR_LUA) == LUA_TNIL)
        return lua_pop(L, 1), NULL;
    lua_len(L, -1);
    const lua_Integer n = lua_tointeger(L, -1);
    struct data *const ret =
        calloc(1, sizeof(*ret) + (size_t)(n + 1) * sizeof(*ret->v));
    if(!ret)
        return lua_pop(L, 2), LOG_ERRNO("calloc"), NULL;
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -2, i + 1);
        lua_geti(L, -1, 1);
        lua_geti(L, -2, 2);
        ret->v[i].name = strdup(lua_tostring(L, -2));
        ret->v[i].base = strdup(lua_tostring(L, -1));
        lua_pop(L, 3);
    }
    lua_pop(L, 2);
    return ret;
}

static bool init(struct battery *d) {
    if(!(d->graph = calloc(GRAPH_WIDTH, sizeof(*d->graph))))
        return LOG_ERRNO("calloc", 0), false;
    char path[CUSTOS_MAX_PATH];
#define X(name, NAME) \
    if(!(join_path(path, 2, d->base, NAME) && (d->name = fopen(path, "r")))) { \
        LOG_ERRNO("fopen(%s" NAME ")", d->base); \
        return false; \
    }
    X(now, "/" NOW)
    X(full, "/" FULL)
    X(design, "/" DESIGN)
    X(status, "/" STATUS)
#undef X
    return true;
}

static void check_value(
    unsigned long *v, unsigned long max, const char *path)
{
    if(*v <= max)
        return;
    LOG_ERR("read invalid value for %s (%f), clamping to %f\n", *v, path, max);
    *v = max;
}

static bool update(
    struct battery *b,
    unsigned long *now, unsigned long *full, unsigned long *design,
    char status[static STATUS_MAX])
{
    if(!rewind_and_scan(b->now, "%lu", now))
        return false;
    if(!rewind_and_scan(b->full, "%lu", full))
        return false;
    if(!rewind_and_scan(b->design, "%lu", design))
        return false;
    size_t n = STATUS_MAX;
    if(!rewind_and_read(b->status, "status", &n, status))
        return false;
    status[n - 1] = 0;
    check_value(full, *design, FULL);
    check_value(now, *full, NOW);
    return true;
}

static void update_graph(size_t n, int i, unsigned long *g, unsigned long x) {
    const size_t max = GRAPH_WIDTH - 1;
    if(!n && i == (int)max)
        memmove(g, g + 1, sizeof(*g) * max);
    g[i] = (g[i] * n + x) / (n + 1);
}

static void render_bar(struct window *w, float charge, float full);
static void render_graph(
    struct window *w, unsigned long max, unsigned long *v, int i);

static void render(
    struct window *w, const char *name, unsigned long full_ul,
    float charge, float full, unsigned long *graph, int graph_i,
    const char *status)
{
    window_print(w, "  ");
    render_bar(w, charge, full);
    window_print(w, " ");
    const float abs = charge / full;
    print_perc(w, abs * 100.0f);
    window_print(w, "|");
    print_perc(w, charge * 100.0f);
    window_printf(w, " %s\n   ", name);
    render_graph(w, full_ul, graph, graph_i);
    window_printf(w, "      %s\n", status);
}

static void render_bar(struct window *w, float charge, float full) {
    window_print(w, "[");
    const float WIDTH = 16;
    unsigned int i = 0;
    for(const unsigned n = (unsigned)(charge * WIDTH); i != n; ++i)
        window_print(w, "=");
    for(const unsigned n = (unsigned)(full * WIDTH); i != n; ++i)
        window_print(w, "-");
    for(const char *c = "|"; i != WIDTH; ++i, c = "-")
        window_print(w, c);
    window_print(w, "]");
}

static void render_graph(
    struct window *w, unsigned long max, unsigned long *v, int i)
{
    enum { W = GRAPH_WIDTH };
    const char bars[][sizeof("█")] =
        {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const int n = ARRAY_SIZE(bars);
    for(int x = 0; x <= i; ++x) {
        const int i = (int)((float)v[x] / (float)max * (float)n);
        window_print(w, bars[MIN(i, n - 1)]);
    }
    window_printf(w, "%*s", W - i - 1, "");
}

void battery_lua(lua_State *L) {
    lua_pushcfunction(L, set_batteries);
    lua_setglobal(L, "batteries");
}

void *battery_init(lua_State *L) {
    struct data *d = get_batteries(L);
    if(!d) {
        if(!(d = calloc(1, sizeof(*d) + 2 * sizeof(*d->v))))
            return LOG_ERRNO("calloc"), NULL;
        d->v->name = strdup("0");
        d->v->base = strdup("/sys/class/power_supply/BAT0");
    }
    if(!init(d->v)) {
        battery_destroy(d);
        return NULL;
    }
    d->graph_i = -1;
    return d;
}

bool battery_destroy(void *p) {
    struct data *const d = p;
    bool ret = true;
    for(struct battery *v = d->v; v->now; ++v) {
#define X(name, NAME) \
    if(v->name && fclose(v->name)) { \
        LOG_ERRNO("fclose(%s" NAME ")", v->base); \
        ret = false; \
    }
        X(now, "/" NOW)
        X(full, "/" FULL)
        X(design, "/" DESIGN)
        X(status, "/" STATUS)
#undef X
        free(v->graph);
        free(v->name);
        free(v->base);
    }
    return ret;
}

bool battery_update(void *p, size_t counter, struct window *w) {
    window_bold_text(w);
    window_print(w, "battery\n");
    window_normal_text(w);
    struct data *const d = p;
    int graph_i = d->graph_i;
    counter %= GRAPH_UPDATE_RATE;
    if(!counter && graph_i != GRAPH_WIDTH - 1)
        d->graph_i = ++graph_i;
    for(struct battery *v = d->v; v->now; ++v) {
        unsigned long now = 0, full = 0, design = 0;
        char status[STATUS_MAX];
        if(!update(v, &now, &full, &design, status))
            return false;
        update_graph(counter, graph_i, v->graph, now);
        render(
            w, v->name, full,
            (float)now / (float)design,
            (float)full / (float)design,
            v->graph, graph_i, status);
    }
    return true;
}

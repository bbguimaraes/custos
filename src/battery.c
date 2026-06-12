#include "battery.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>

#include "graph.h"
#include "utils.h"
#include "window.h"

#define NOW "energy_now"
#define FULL "energy_full"
#define DESIGN "energy_full_design"
#define STATUS "status"
#define VAR_LUA "battery.batteries"

#define STATUS_MAX 32
#define GRAPH_UPDATE_RATE (15 * 60)

typedef unsigned long graph_type;

struct battery {
    char *name, *base;
    FILE *now, *full, *design, *status;
};

struct data {
    struct graph graph;
    graph_type *graph_data;
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
        return lua_pop(L, 3), LOG_ERRNO("calloc"), NULL;
    graph_lua_read(&ret->graph, L);
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -3, i + 1);
        lua_geti(L, -1, 1);
        lua_geti(L, -2, 2);
        ret->v[i].name = strdup(lua_tostring(L, -2));
        ret->v[i].base = strdup(lua_tostring(L, -1));
        lua_pop(L, 3);
    }
    lua_pop(L, 3);
    return ret;
}

static bool init(struct battery *v) {
    for(; v->name; ++v) {
        char path[CUSTOS_MAX_PATH];
#define X(name, NAME) \
            if(!( \
                join_path(path, 2, v->base, NAME) \
                && (v->name = fopen(path, "r")) \
            )) { \
                LOG_ERRNO("fopen(%s" NAME ")", v->base); \
                return false; \
            }
        X(now, "/" NOW)
        X(full, "/" FULL)
        X(design, "/" DESIGN)
        X(status, "/" STATUS)
#undef X
    }
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

static void graph_acc(void *p, const void *x, size_t n) {
    graph_type *const tp = (graph_type*)p;
    const graph_type tx = *(const graph_type*)x;
    *tp = (*tp * n + tx) / (n + 1);
}

static float graph_idx(void *p, int i) {
    return (float)((graph_type*)p)[i];
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

static void render(
    struct window *w, const char *name, unsigned long full_ul,
    float charge, float full, unsigned long *graph, const struct graph *g,
    const char *status)
{
    window_print(w, "  ");
    print_bar(w, charge, full);
    window_print(w, " ");
    const float abs = charge / full;
    print_perc(w, abs * 100.0f);
    window_print(w, "|");
    print_perc(w, charge * 100.0f);
    window_printf(w, " %s %s", status, name);
    graph_render(w, g, full_ul, graph, graph_idx);
    window_print(w, "\n");
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
    size_t n = 0;
    for(struct battery *v = d->v; v->name; ++v)
        ++n;
    d->graph.i = -1;
    if(!d->graph.w)
        d->graph.w = 24;
    if(!d->graph.h)
        d->graph.h = 2;
    if(!(
        checked_calloc_p(
            n * (size_t)d->graph.w,
            sizeof(*d->graph_data), (void**)&d->graph_data)
        && init(d->v)
    )) {
        battery_destroy(d);
        return NULL;
    }
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
        free(v->name);
        free(v->base);
    }
    free(d->graph_data);
    free(d);
    return ret;
}

bool battery_update(void *p, size_t counter, struct window *w) {
    window_bold_text(w);
    window_print(w, "battery\n");
    window_normal_text(w);
    struct data *const d = p;
    counter %= GRAPH_UPDATE_RATE;
    if(!counter && d->graph.i != d->graph.w - 1)
        ++d->graph.i;
    graph_type *graph = d->graph_data;
    for(struct battery *v = d->v; v->now; ++v, graph += d->graph.w) {
        unsigned long now = 0, full = 0, design = 0;
        char status[STATUS_MAX];
        if(!update(v, &now, &full, &design, status))
            return false;
        graph_update(
            &d->graph, counter, graph, &now, sizeof(graph_type), graph_acc);
        render(
            w, v->name, full,
            (float)now / (float)design,
            (float)full / (float)design,
            graph, &d->graph, status);
    }
    return true;
}

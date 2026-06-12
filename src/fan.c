#include "fan.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <lua.h>

#include "graph.h"
#include "utils.h"
#include "window.h"

#define VAR_LUA "fans.inputs"

#define STATUS_MAX_LEN sizeof("disabled")
#define LEVEL_MAX_LEN sizeof("full-speed")
#define GRAPH_UPDATE_RATE 5

static const u16 INVALID_READING = 65535;

typedef u16 graph_type;

struct fan {
    FILE *f;
    char *path, *label;
    graph_type max;
};

struct data {
    struct graph graph;
    graph_type *graph_data;
    struct fan v[];
};

// TODO strata
static char *string_find_char(char *s, char c) {
    while(*s && *s != c)
        ++s;
    return s;
}

static int set_fans(lua_State *L) {
    lua_setglobal(L, VAR_LUA);
    return 0;
}

static struct data *get_inputs(lua_State *L) {
    if(lua_getglobal(L, VAR_LUA) == LUA_TNIL)
        return lua_pop(L, 1), NULL;
    lua_len(L, -1);
    const lua_Integer n = lua_tointeger(L, -1);
    struct data *const ret =
        calloc(1, sizeof(*ret) + (size_t)(n + 1) * sizeof(*ret->v));
    if(!ret)
        return lua_pop(L, 2), LOG_ERRNO("calloc"), NULL;
    graph_lua_read(&ret->graph, L);
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -3, i + 1);
        lua_geti(L, -1, 1);
        lua_geti(L, -2, 2);
        ret->v[i].label = strdup(lua_tostring(L, -2));
        ret->v[i].path = strdup(lua_tostring(L, -1));
        if(lua_getfield(L, -3, "max") != LUA_TNIL)
            ret->v[i].max = (u16)lua_tointeger(L, -1);
        lua_pop(L, 4);
    }
    lua_pop(L, 3);
    return ret;
}

static bool init(struct fan *v) {
    for(; v->path; ++v)
        if(!(v->f = fopen(v->path, "r")))
            return false;
    return true;
}

static char *parse_field(char **s_p, const char *name, size_t n, size_t max) {
    char *s = *s_p;
    if(strncmp(s, name, n) != 0)
        goto err;
    s += n;
    char *const p = string_find_char(s, '\n');
    if(!*p || (max && max < (size_t)(p - s)))
        goto err;
    *p = 0;
    *s_p = p + 1;
    return s;
err:
    LOG_ERR("failed to parse field \"%s\"\n", name);
    return NULL;
}

static bool parse_file(
    const struct fan *f, char *s, char *status, u16 *speed, char *level)
{
    const char s0[] = "status:\t\t";
    char *p;
    if(!(p = parse_field(&s, s0, sizeof(s0) - 1, STATUS_MAX_LEN)))
        goto err;
    strcpy(status, p);
    const char s1[] = "speed:\t\t";
    if(!(p = parse_field(&s, s1, sizeof(s1) - 1, 0)))
        goto err;
    if(sscanf(p, "%" SCNu16, speed) != 1)
        goto err;
    if(f->max < *speed) {
        if(*speed == INVALID_READING) {
            *speed = f->max;
            LOG_ERR(
                "hardware error when reading speed, truncating to %"
                PRIu16 "\n",
                *speed);
        } else {
            LOG_ERR(
                "invalid speed: %" PRIu16 " < %" PRIu16 "\n",
                f->max, *speed);
            goto err;
        }
    }
    const char s2[] = "level:\t\t";
    if(!(p = parse_field(&s, s2, sizeof(s2) - 1, LEVEL_MAX_LEN)))
        goto err;
    strcpy(level, p);
    return true;
err:
    LOG_ERR("unexpected content in file %s\n", f->path);
    return false;
}

static void graph_acc(void *p, const void *x, size_t n) {
    graph_type *const tp = (graph_type*)p;
    const graph_type tx = *(const graph_type*)x;
    *tp = (graph_type)((*tp * n + tx) / (n + 1));
}

static float graph_idx(void *p, int i) {
    return ((graph_type*)p)[i];
}

static bool update(
    const struct fan *f,
    u16 *speed, char status[static STATUS_MAX_LEN],
    char level[static LEVEL_MAX_LEN])
{
    char buffer[1024];
    size_t nr = sizeof(buffer) - 1;
    if(!rewind_and_read(f->f, "contents", &nr, buffer))
        return false;
    buffer[nr] = 0;
    if(!parse_file(f, buffer, status, speed, level))
        return false;
    return true;
}

void render(
    struct window *w, const char *label, const char *status, const char *level,
    u16 speed, u16 max, u16 *graph, const struct graph *g)
{
    window_print(w, "  ");
    print_bar(w, max ? 100.0f * (float)speed / (float)max : 0);
    window_printf(w, " %uRPM %s %s %s\n", speed, level, status, label);
    graph_render(w, g, max, graph, graph_idx);
    window_print(w, "\n");
}

void fan_lua(struct lua_State *L) {
    lua_pushcfunction(L, set_fans);
    lua_setglobal(L, "fans");
}

void *fan_init(struct lua_State *L) {
    struct data *const d = get_inputs(L);
    if(!d)
        return NULL;
    size_t n = 0;
    for(struct fan *v = d->v; v->path; ++v)
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
        fan_destroy(d);
        return NULL;
    }
    return d;
}

bool fan_destroy(void *p) {
    struct data *const d = p;
    if(!d)
        return true;
    bool ret = true;
    for(struct fan *v = d->v; v->f; ++v) {
        ret = close_file(v->f, v->path);
        free(v->path);
        free(v->label);
    }
    free(d->graph_data);
    free(d);
    return ret;
}

bool fan_update(void *p, size_t counter, struct window *w) {
    window_bold_text(w);
    window_print(w, "fan\n");
    window_normal_text(w);
    struct data *const d = p;
    counter %= GRAPH_UPDATE_RATE;
    if(!counter && d->graph.i != d->graph.w - 1)
        ++d->graph.i;
    graph_type *graph = d->graph_data;
    for(struct fan *v = d->v; v->f; ++v, graph += d->graph.w) {
        u16 speed;
        char status[STATUS_MAX_LEN], level[LEVEL_MAX_LEN];
        if(!update(v, &speed, status, level))
            return false;
        graph_update(
            &d->graph, counter, graph, &speed, sizeof(graph_type), graph_acc);
        render(w, v->label, status, level, speed, v->max, graph, &d->graph);
    }
    return true;
}

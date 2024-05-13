#include "battery.h"

#include <stdio.h>
#include <stdlib.h>

#include <lua.h>

#include "utils.h"

#define NOW "/energy_now"
#define FULL "/energy_full"
#define DESIGN "/energy_full_design"
#define STATUS "/status"
#define VAR_LUA "battery.batteries"

#define STATUS_MAX 32

struct battery {
    char *name, *base;
    FILE *now, *full, *design, *status;
};

struct data {
    int unused;
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
    char path[CUSTOS_MAX_PATH];
#define X(name, NAME) \
    if(!(join_path(path, 2, d->base, NAME) && (d->name = fopen(path, "r")))) { \
        LOG_ERRNO("fopen(%s" NAME ")", d->base); \
        return false; \
    }
    X(now, NOW)
    X(full, FULL)
    X(design, DESIGN)
    X(status, STATUS)
#undef X
    return true;
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
    return true;
}

static void render_bar(float charge, float full);

static void render(
    const char *name, float charge, float full, const char *status)
{
    printf("  %s: %s ", name, status);
    const float abs = charge / full;
    if(abs == 1.0f)
        fputs("100%", stdout);
    else
        printf("%2.2f%%", 100.0f * abs);
    putchar('|');
    if(charge == 1.0f)
        fputs("100%", stdout);
    else
        printf("%2.2f%%", 100.0f * charge);
    fputs(" [", stdout);
    render_bar(charge, full);
    printf("] (%s)\n", status);
}

static void render_bar(float charge, float full) {
    const float WIDTH = 21;
    unsigned int i = 0;
    for(const unsigned n = (unsigned)(charge * WIDTH); i != n; ++i)
        putchar('=');
    for(const unsigned n = (unsigned)(full * WIDTH); i != n; ++i)
        putchar('-');
    for(char c = '|'; i != WIDTH; ++i, c = '-')
        putchar(c);
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
        X(now, NOW)
        X(full, FULL)
        X(design, DESIGN)
        X(status, STATUS)
#undef X
        free(v->name);
        free(v->base);
    }
    return ret;
}

bool battery_update(void *p, size_t counter) {
    (void)counter;
    puts("battery");
    struct data *const d = p;
    for(struct battery *v = d->v; v->now; ++v) {
        unsigned long now = 0, full = 0, design = 0;
        char status[STATUS_MAX];
        if(!update(v, &now, &full, &design, status))
            return false;
        render(
            v->name,
            (float)now / (float)design,
            (float)full / (float)design,
            status);
    }
    return true;
}

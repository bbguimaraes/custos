#include "backlight.h"

#include <stdio.h>
#include <stdlib.h>

#include <lua.h>

#include "utils.h"

#define VAR_LUA "backlight.backlights"

struct source {
    char *name, *cur_path, *max_path;
    FILE *cur, *max;
};

static int set_backlights(lua_State *L) {
    lua_setglobal(L, VAR_LUA);
    return 0;
}

void backlight_lua(struct lua_State *L) {
    lua_pushcfunction(L, set_backlights);
    lua_setglobal(L, "backlights");
}

static struct source *get_backlights(lua_State *L) {
    if(lua_getglobal(L, VAR_LUA) == LUA_TNIL)
        return lua_pop(L, 1), NULL;
    lua_len(L, -1);
    const lua_Integer n = lua_tointeger(L, -1);
    struct source *const ret = calloc(1, (size_t)(n + 1) * sizeof(*ret));
    if(!ret)
        return lua_pop(L, 2), LOG_ERRNO("calloc"), NULL;
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -2, i + 1);
        lua_geti(L, -1, 1);
        lua_geti(L, -2, 2);
        ret[i].name = strdup(lua_tostring(L, -2));
        ret[i].cur_path = strdup(lua_tostring(L, -1));
        lua_pop(L, 3);
    }
    lua_pop(L, 2);
    return ret;
}

void *backlight_init(struct lua_State *L) {
    struct source *ret = get_backlights(L);
    if(!ret) {
        ret = calloc(2, sizeof(*ret));
        if(!ret)
            return LOG_ERRNO("calloc", 0), NULL;
        if(!(ret->name = strdup("intel_backlight"))) {
            LOG_ERRNO("strdup", 0);
            goto err;
        }
        if(!(ret->cur_path = strdup(
            "/sys/class/backlight/intel_backlight/brightness"
        ))) {
            LOG_ERRNO("strdup", 0);
            goto err;
        }
    }
    char tmp[CUSTOS_MAX_PATH];
    for(struct source *v = ret; v->name; ++v) {
        strcpy(tmp, v->cur_path);
        strcpy(strrchr(tmp, '/') + 1, "max_brightness");
        if(!(v->cur = fopen(v->cur_path, "r"))) {
            LOG_ERRNO("fopen(%s)", v->cur_path);
            goto err;
        }
        if(!(v->max = fopen(tmp, "r"))) {
            LOG_ERRNO("fopen(%s)", tmp);
            goto err;
        }
        v->max_path = strdup(tmp);
    }
    return ret;
err:
    backlight_destroy(ret);
    return NULL;
}

bool backlight_destroy(void *d) {
    bool ret = true;
    for(struct source *v = d; v->name; ++v) {
        if(v->cur)
            ret = close_file(v->cur, v->cur_path) && ret;
        if(v->max)
            ret = close_file(v->max, v->max_path) && ret;
        free(v->name);
        free(v->cur_path);
        free(v->max_path);
    }
    free(d);
    return ret;
}

bool backlight_update(void *d, size_t counter) {
    (void)counter;
    puts("backlight");
    for(struct source *v = d; v->name; ++v) {
        int cur, max;
        if(!rewind_and_scan(v->cur, "%d", &cur))
            return false;
        if(!rewind_and_scan(v->max, "%d", &max))
            return false;
        fputs("  ", stdout);
        const float b = 100.0f * (float)cur / (float)max;
        print_bar(b);
        printf(" %d%% %s\n", (int)b, v->name);
    }
    return true;
}

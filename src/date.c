#include "date.h"

#include <errno.h>
#include <stdio.h>

#include <time.h>

#include <lua.h>

#include "utils.h"

#define DATE_FMT "%Y-%m-%dT%H:%M:%S"
#define DATE_SIZE sizeof("YYYY-MM-DDTHH:MM:SS")
#define VAR_LUA "date.timezones"

struct tz {
    long offset;
    char *name, *id;
};

static bool init_tzs(struct tz *v);
static bool restore_tz(const char *tz);

static int set_timezones(lua_State *L) {
    lua_setglobal(L, VAR_LUA);
    return 0;
}

static struct tz *get_timezones(lua_State *L) {
    if(lua_getglobal(L, VAR_LUA) == LUA_TNIL)
        return lua_pop(L, 1), NULL;
    lua_len(L, 1);
    const lua_Integer n = lua_tointeger(L, -1);
    struct tz *const v = calloc((size_t)(n + 1), sizeof(*v));
    if(!v)
        return lua_pop(L, 2), LOG_ERRNO("calloc"), NULL;
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -2, i + 1);
        lua_geti(L, -1, 1);
        lua_geti(L, -2, 2);
        v[i] = (struct tz){
            .name = strdup(lua_tostring(L, -2)),
            .id = strdup(lua_tostring(L, -1)),
        };
        lua_pop(L, 3);
    }
    lua_pop(L, 2);
    return v;
}

void date_lua(lua_State *L) {
    lua_pushcfunction(L, set_timezones);
    lua_setglobal(L, "timezones");
}

void *date_init(lua_State *L) {
    struct tz *v = get_timezones(L);
    if(!v) {
        if(!(v = calloc(2, sizeof(*v))))
            return LOG_ERRNO("calloc"), NULL;
        *v = (struct tz){.name = strdup("UTC"), .id = strdup("UTC")};
    }
    const char *const tz = getenv("TZ");
    if(!init_tzs(v) || !restore_tz(tz))
        return date_destroy(v), NULL;
    return v;
}

static bool init_tzs(struct tz *v) {
    time_t t = {0};
    if(time(&t) == -1)
        return LOG_ERRNO("time"), false;
    for(; v->name; ++v) {
        if(setenv("TZ", v->id, 1) == -1)
            return LOG_ERRNO("setenv"), false;
        tzset();
        struct tm tm = {0};
        if(!localtime_r(&t, &tm))
            return LOG_ERRNO("localtime_r"), false;
        tm.tm_isdst = 0;
        time_t t_tz = mktime(&tm);
        if(t_tz == -1)
            return LOG_ERRNO("mktime"), false;
        v->offset = t - t_tz + timezone;
    }
    return true;
}

static bool restore_tz(const char *tz) {
    if(tz) {
        if(setenv("TZ", tz, 1) == -1)
            return LOG_ERRNO("setenv"), false;
    } else
        if(unsetenv("TZ") == -1)
            return LOG_ERRNO("unsetenv"), false;
    tzset();
    return true;
}

bool date_destroy(void *d) {
    for(struct tz *v = d; v->name; ++v)
        free(v->name), free(v->id);
    free(d);
    return true;
}

bool date_update(void *d) {
    puts("date");
    time_t t = {0};
    if(time(&t) == -1)
        return LOG_ERRNO("time"), false;
    for(const struct tz *v = d; v->name; ++v) {
        const time_t t_tz = t - v->offset;
        struct tm tm = {0};
        if(!gmtime_r(&t_tz, &tm))
            return LOG_ERRNO("gmtime_r"), false;
        char str[DATE_SIZE] = {0};
        if(strftime(str, sizeof(str), DATE_FMT, &tm) != sizeof(str) - 1)
            return LOG_ERR("strftime failed\n"), false;
        printf("  %s %s\n", v->name, str);
    }
    return true;
}

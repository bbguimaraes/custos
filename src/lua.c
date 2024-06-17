#include "custos.h"

#include <glob.h>

#include <lualib.h>
#include <lauxlib.h>

#include "mod.h"
#include "utils.h"
#include "window.h"

#define G "custos"

struct data {
    u8 *enabled_modules;
    struct window **windows;
    size_t ***modules, *n_windows;
};

struct data *get_global(lua_State *L) {
    lua_getglobal(L, G);
    struct data *const ret = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ret;
}

static int nop(lua_State *L) { (void)L; return 0; }

static lua_Integer len(lua_State *L, int i) {
    lua_len(L, i);
    const lua_Integer ret = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return ret;
}

static struct module *find_module(struct module *v, size_t n, const char *name)
{
    return bsearch(
        &(struct module){.name = name}, v, n, sizeof(*v), module_name_cmp);
}

static bool is_enabled(struct module *v, struct module *p, u8 enabled) {
    return enabled & (1u << (p - v));
}

static int glob_lua(lua_State *L) {
    const char *pattern = luaL_checkstring(L, 1);
    glob_t paths;
    switch(glob(pattern, 0, NULL, &paths)) {
    case GLOB_NOMATCH: return 0;
    case GLOB_NOSPACE: return luaL_error(L, "glob(%s): NOSPACE\n", pattern);
    case GLOB_ABORTED: return luaL_error(L, "glob(%s): ABORTED\n", pattern);
    }
    for(size_t i = 0; i != paths.gl_pathc; ++i)
        lua_pushstring(L, paths.gl_pathv[i]);
    globfree(&paths);
    return (int)paths.gl_pathc;
}

static int set_enabled_modules(lua_State *L) {
    size_t n_modules;
    struct module *const modules = get_modules(&n_modules);
    struct data *const d = get_global(L);
    u8 enabled = 0;
    const lua_Integer n = len(L, 1);
    for(lua_Integer i = 1; i <= n; ++i) {
        lua_geti(L, 1, i);
        const char *const name = lua_tostring(L, -1);
        const struct module *const m = find_module(modules, n_modules, name);
        if(!m)
            return luaL_error(L, "invalid module: %s", name);
        enabled |= (u8)(1u << (m - modules));
        lua_pop(L, 1);
    }
    if(enabled)
        *d->enabled_modules = enabled;
    return 0;
}

static bool load_window(
    lua_State *L, struct module *modules, size_t n, u8 enabled_modules,
    struct window *w, size_t **m);

static int set_windows(lua_State *L) {
    struct data *const d = get_global(L);
    size_t n_modules;
    struct module *const modules = get_modules(&n_modules);
    const u8 enabled = *d->enabled_modules;
    lua_len(L, 1);
    const lua_Integer n = lua_tointeger(L, -1);
    struct window *const ws = checked_calloc((size_t)n + 1, sizeof(*ws));
    if(!ws)
        return luaL_error(L, "%s", __func__);
    size_t **const ms = checked_calloc((size_t)n + 1, sizeof(*ms));
    if(!ms)
        goto e0;
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, 1, i + 1);
        if(!load_window(L, modules, n_modules, enabled, ws + i, ms + i))
            goto e1;
        lua_pop(L, 1);
    }
    *d->windows = ws;
    *d->modules = ms;
    *d->n_windows = (size_t)n;
    return 0;
e1:
    for(size_t **v = ms; *v; ++v)
        free(*v);
    free(ms);
e0:
    free(ws);
    return luaL_error(L, "%s", __func__);
}

static bool load_window_modules(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled,
    size_t **m);

static bool load_window(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled_modules,
    struct window *w, size_t **m)
{
    if(lua_getfield(L, -1, "x") != LUA_TNIL)
        w->x = (int)lua_tointeger(L, -1);
    if(lua_getfield(L, -2, "y") != LUA_TNIL)
        w->y = (int)lua_tointeger(L, -1);
    if(lua_getfield(L, -3, "modules") != LUA_TNIL)
        load_window_modules(L, modules, n_modules, enabled_modules, m);
    lua_pop(L, 3);
    return true;
}

static size_t count_enabled(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled,
    lua_Integer n, int t);
static void add_modules(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled,
    lua_Integer n, size_t *m);

static bool load_window_modules(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled,
    size_t **m)
{
    const lua_Integer n = len(L, -1);
    const size_t ne = count_enabled(L, modules, n_modules, enabled, n, -1);
    size_t *const mi = checked_calloc((size_t)ne + 1, sizeof(*mi));
    if(!mi)
        return false;
    add_modules(L, modules, n_modules, enabled, n, mi);
    *m = mi;
    return true;
}

static size_t count_enabled(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled,
    lua_Integer n, int t)
{
    size_t ret = 0;
    for(int i = 0; i != n; ++i) {
        lua_geti(L, t, i + 1);
        const char *const name = lua_tostring(L, -1);
        struct module *const m = find_module(modules, n_modules, name);
        if(!m)
            return (size_t)luaL_error(L, "unknown module: %s", name);
        lua_pop(L, 1);
        ret += is_enabled(modules, m, enabled);
    }
    return ret;
}

static void add_modules(
    lua_State *L, struct module *modules, size_t n_modules, u8 enabled,
    lua_Integer n, size_t *m)
{
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -1, i + 1);
        struct module *const mi =
            find_module(modules, n_modules, lua_tostring(L, -1));
        if(is_enabled(modules, mi, enabled))
            *m++ = (size_t)(mi - modules);
        lua_pop(L, 1);
    }
    *m = (size_t)-1;
}

static void prepare_config_load(lua_State *L, struct data *d) {
    lua_pushlightuserdata(L, d);
    lua_setglobal(L, G);
    lua_pushcfunction(L, set_windows);
    lua_setglobal(L, "windows");
    lua_pushcfunction(L, *d->enabled_modules ? nop : set_enabled_modules);
    lua_setglobal(L, "modules");
    size_t n;
    struct module *const modules = get_modules(&n);
    for(size_t i = 0; i != n; ++i) {
        struct module *const m = modules + i;
        if(m->lua)
            m->lua(L);
    }
}

lua_State *custos_lua_init(void) {
    lua_State *const L = luaL_newstate();
    if(!L)
        return LOG_ERR("failed to create Lua state\n"), NULL;
    luaL_openlibs(L);
    lua_pushcfunction(L, glob_lua);
    lua_setglobal(L, "glob");
    return L;
}

void custos_lua_destroy(struct lua_State *L) {
    lua_close(L);
}

bool custos_load_config(
    struct lua_State *L, uint8_t *enabled_modules, struct window **windows,
    size_t *n_windows, size_t ***modules)
{
    enum { MAX_PATH = 4096 };
    char path[MAX_PATH] = {0};
#define B "/custos/init.lua"
    const char *const base = B;
    const char *const home_base = "/.config" B;
#undef B
    const char *const xdg = getenv("XDG_CONFIG_HOME");
    if(xdg) {
        if(!join_path(path, 2, xdg, base))
            return false;
    } else {
        const char *const home = getenv("HOME");
        if(!home)
            return true;
        if(!join_path(path, 2, home, home_base))
            return false;
    }
    if(!file_exists(path))
        return true;
    struct data d = {
        .enabled_modules = enabled_modules,
        .windows = windows,
        .modules = modules,
        .n_windows = n_windows,
    };
    prepare_config_load(L, &d);
    if(luaL_loadfile(L, path) != LUA_OK) {
        LOG_ERR("%s: %s\n", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    if(lua_pcall(L, 0, 0, 0) != LUA_OK) {
        LOG_ERR("%s: %s\n", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

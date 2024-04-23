#include "custos.h"

#include <glob.h>

#include <lualib.h>
#include <lauxlib.h>

#include "mod.h"
#include "utils.h"

#define G "custos"

struct data {
    u8 *enabled_modules;
    size_t n_modules;
    struct module *modules;
};

struct data *get_global(lua_State *L) {
    lua_getglobal(L, G);
    struct data *const ret = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ret;
}

static int nop(lua_State *L) { (void)L; return 0; }

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
    struct data *const d = get_global(L);
    u8 enabled = 0;
    lua_len(L, 1);
    const lua_Integer n = lua_tointeger(L, -1);
    lua_pop(L, 1);;
    for(lua_Integer i = 1; i <= n; ++i) {
        lua_geti(L, 1, i);
        const struct module key = {.name = lua_tostring(L, -1)};
        const struct module *const m = bsearch(
            &key, d->modules, d->n_modules, sizeof(struct module),
            module_name_cmp);
        if(!m)
            luaL_error(L, "invalid module: %s", key.name);
        enabled |= (u8)(1u << (m - d->modules));
        lua_pop(L, 1);
    }
    if(enabled)
        *d->enabled_modules = enabled;
    return 0;
}

static void prepare_config_load(lua_State *L, struct data *d) {
    lua_pushlightuserdata(L, d);
    lua_setglobal(L, G);
    lua_pushcfunction(L, *d->enabled_modules ? nop : set_enabled_modules);
    lua_setglobal(L, "modules");
    struct module *const modules = d->modules;
    const size_t n = d->n_modules;
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

bool custos_load_config(struct lua_State *L, u8 *enabled_modules) {
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
        .modules = get_modules(&d.n_modules),
    };
    prepare_config_load(L, &d);
    if(luaL_dofile(L, path) != LUA_OK) {
        LOG_ERR("%s: %s\n", path, lua_tostring(L, -1));
        lua_pop(L, 1);
        return false;
    }
    return true;
}

#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/statvfs.h>

#include <lua.h>

#include "utils.h"
#include "window.h"

#define VAR_LUA "fs.file_systems"
#define UPDATE_RATE 60

struct fs {
    char *name;
    float use;
    size_t free, total;
};

static int set_file_systems(lua_State *L) {
    lua_setglobal(L, VAR_LUA);
    return 0;
}

static struct fs *get_file_systems(lua_State *L) {
    const lua_Integer n = (lua_getglobal(L, VAR_LUA) == LUA_TNIL)
        ? 0 : (lua_len(L, -1), lua_tointeger(L, -1));
    struct fs *const ret = calloc((size_t)(n + 1), sizeof(*ret));
    if(!ret)
        return NULL;
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -2, i + 1);
        char *const s = strdup(lua_tostring(L, -1));
        if(!s)
            goto err;
        ret[i].name = s;
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
    return ret;
err:
    for(struct fs *v = ret; v->name; ++v)
        free(v->name);
    free(ret);
    return NULL;
}

bool update_fs(struct fs *p) {
    struct statvfs s;
    if(statvfs(p->name, &s))
        return LOG_ERRNO("statfs(%s)\n", p->name), false;
    const fsblkcnt_t free = s.f_blocks - s.f_bavail;
    p->use = (float)free / (float)s.f_blocks * 100.0f;
    p->free = (size_t)(free * s.f_bsize);
    p->total = (size_t)(s.f_blocks * s.f_bsize);
    return true;
}

void fs_lua(struct lua_State *L) {
    lua_pushcfunction(L, set_file_systems);
    lua_setglobal(L, "file_systems");
}

void *fs_init(struct lua_State *L) {
    return get_file_systems(L);
}

bool fs_destroy(void *p) {
    for(struct fs *v = p; v->name; ++v)
        free(v->name);
    free(p);
    return true;
}

bool fs_update(void *d, size_t counter, struct window *w) {
    window_bold_text(w);
    window_print(w, "fs\n");
    window_normal_text(w);
    const bool update = !(counter % UPDATE_RATE);
    for(struct fs *v = d; v->name; ++v) {
        if(update && !update_fs(v))
            return false;
        window_print(w, "  ");
        print_bar(w, v->use);
        window_print(w, " ");
        print_perc(w, v->use);
        window_print(w, " ");
        print_size(w, v->free);
        window_print(w, "/");
        print_size(w, v->total);
        window_printf(w, " %s\n", v->name);
    }
    return true;
}

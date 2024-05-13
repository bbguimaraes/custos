#include "thermal.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glob.h>

#include <lua.h>

#include "utils.h"

#define VAR_LUA "thermal.inputs"

struct data {
    FILE *input;
    char *path, *label;
};

static char *label_path(const char *path);
static char *read_label(const char *path);

static bool init(struct data *d) {
    if(!(d->input = open_file(d->path, "r")))
        return false;
    char *const label = label_path(d->path);
    if(!label)
        return false;
    d->label = read_label(label);
    free(label);
    return d->label;
}

static char *replace(const char *s, const char *repl);

static char *label_path(const char *path) {
    const char repl[] = "label";
    static_assert(sizeof(repl) <= sizeof("input"));
    return replace(path, repl);
}

static char *replace(const char *s, const char *repl) {
    const char src[] = "input";
    assert(strlen(repl) + 1 <= sizeof(repl));
    const size_t n = strlen(s) - sizeof(src) + 1;
    assert(strcmp(s + n, src) == 0);
    char *const ret = strdup(s);
    if(!ret)
        return LOG_ERRNO("strdup"), NULL;
    strcpy(ret + n, repl);
    return ret;
}

static char *read_label(const char *path) {
    FILE *const f = fopen(path, "r");
    if(!f)
        return LOG_ERRNO("fopen(%s)", path), NULL;
    char buffer[1024];
    const size_t n = fread(buffer, 1, sizeof(buffer), f);
    if(!n) {
        LOG_ERRNO("fread");
        goto err;
    }
    if(fclose(f) == EOF)
        return LOG_ERRNO("fclose(%s)", path), NULL;
    enum { new_line = 1 };
    buffer[n - new_line] = 0;
    return strdup(buffer);
err:
    if(fclose(f) == EOF)
        LOG_ERRNO("fclose(%s)", path);
    return NULL;
}

static struct data *init_from_glob(const char *pat) {
    glob_t paths = {0};
    switch(glob(pat, 0, NULL, &paths)) {
    case GLOB_NOMATCH: LOG_ERR("glob(%s): no files found\n", pat); return NULL;
    case GLOB_NOSPACE: LOG_ERR("glob(%s): NOSPACE\n", pat); return NULL;
    case GLOB_ABORTED: LOG_ERR("glob(%s): ABORTED\n", pat); return NULL;
    }
    struct data *v = calloc(paths.gl_pathc + 1, sizeof(*v));
    if(!v) {
        LOG_ERRNO("calloc");
        goto e0;
    }
    for(size_t i = 0; i != paths.gl_pathc; ++i)
        if(!(v[i].path = strdup(paths.gl_pathv[i]))) {
            LOG_ERRNO("strdup");
            goto e1;
        }
    globfree(&paths);
    return v;
e1:
    thermal_destroy(v);
e0:
    globfree(&paths);
    return NULL;
}

static int set_inputs(lua_State *L) {
    lua_setglobal(L, VAR_LUA);
    return 0;
}

struct data *get_inputs(lua_State *L) {
    if(lua_getglobal(L, VAR_LUA) == LUA_TNIL)
        return lua_pop(L, 1), NULL;
    lua_len(L, 1);
    const lua_Integer n = lua_tointeger(L, -1);
    struct data *const v = calloc((size_t)(n + 1), sizeof(*v));
    if(!v)
        return lua_pop(L, 2), LOG_ERRNO("calloc"), NULL;
    for(lua_Integer i = 0; i != n; ++i) {
        lua_geti(L, -2, i + 1);
        v[i].path = strdup(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
    return v;
}

void thermal_lua(lua_State *L) {
    lua_pushcfunction(L, set_inputs);
    lua_setglobal(L, "thermal");
}

void *thermal_init(struct lua_State *L) {
    struct data *v = get_inputs(L);
    if(!v)
        v = init_from_glob(
            "/sys/devices/platform/coretemp.0/hwmon/*/temp*_input");
    if(!v)
        return NULL;
    for(struct data *p = v; p->path; ++p)
        if(!init(p))
            return thermal_destroy(v), NULL;
    return v;
}

bool thermal_destroy(void *d) {
    if(!d)
        return true;
    bool ret = true;
    for(struct data *v = d; v->path; ++v) {
        if(v->input && !close_file(v->input, v->path))
            ret = false;
        free(v->path);
        free(v->label);
    }
    free(d);
    return ret;
}

bool thermal_update(void *d, size_t counter) {
    (void)counter;
    puts("thermal");
    for(struct data *v = d; v->input; ++v) {
        int temp = 0;
        if(!rewind_and_scan(v->input, "%d", &temp))
            return false;
        printf("  %.1f°C %s\n", (double)temp / 1e3, v->label);
    }
    return true;
}

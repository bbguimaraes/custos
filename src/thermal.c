#include "thermal.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glob.h>
#include "utils.h"

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

void *thermal_init(void) {
    struct data *v = init_from_glob(
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

bool thermal_update(void *d) {
    puts("thermal");
    for(struct data *v = d; v->input; ++v) {
        int temp = 0;
        if(!rewind_and_scan(v->input, "%d", &temp))
            return false;
        printf("  %.1f°C %s\n", (double)temp / 1e3, v->label);
    }
    return true;
}

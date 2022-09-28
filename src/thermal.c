#include "thermal.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glob.h>
#include "utils.h"

struct data {
    FILE *input;
    char *path;
};

static bool init(struct data *d) {
    return (d->input = open_file(d->path, "r"));
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
    }
    free(d);
    return ret;
}

bool thermal_update(void *d) {
    puts("thermal");
    size_t i = 0;
    for(struct data *v = d; v->input; ++v) {
        int temp = 0;
        if(!rewind_and_scan(v->input, "%d", &temp))
            return false;
        printf("  %zu: %.1f°C\n", i++, (double)temp / 1e3);
    }
    return true;
}

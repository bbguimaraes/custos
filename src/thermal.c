#include "thermal.h"

#include <assert.h>
#include <errno.h>
#include <glob.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static const char *PATTERN =
    "/sys/devices/platform/coretemp.0/hwmon/*/temp*_input";

struct entry {
    FILE *f;
    char *path, *label;
};

static char *label_path(const char *path) {
    const size_t len = strlen(path);
    const char *const e = path + len;
    const char *const repl = e - (sizeof("input") - 1);
    assert(strcmp(repl, "input") == 0);
    char *const label = strdup(path);
    if(label)
        strcpy(label + (repl - path), "label");
    else
        LOG_ERRNO("strdup");
    return label;
}

static bool read_label(const char *path, char **out) {
    char *const label = label_path(path);
    if(!label)
        return false;
    bool ret = false;
    FILE *const f = fopen(label, "r");
    if(!f) {
        LOG_ERRNO("fopen(%s)", label);
        goto err;
    }
    char buffer[1024] = {0};
    size_t n = fread(buffer, 1, sizeof(buffer), f);
    if(!n) {
        LOG_ERRNO("fread");
        goto err;
    }
    if(fclose(f) == EOF) {
        LOG_ERRNO("fclose");
        goto err;
    }
    buffer[n == sizeof(buffer) ? n - 2 : n - 1] = 0;
    *out = strdup(buffer);
    ret = true;
err:
    free(label);
    return ret;
}

void *thermal_init(void) {
    const char *const pat = PATTERN;
    glob_t paths = {0};
    switch(glob(pat, 0, NULL, &paths)) {
    case GLOB_NOMATCH: LOG_ERR("glob(%s): no files found\n", pat); return NULL;
    case GLOB_NOSPACE: LOG_ERR("glob(%s): NOSPACE\n", pat); return NULL;
    case GLOB_ABORTED: LOG_ERR("glob(%s): ABORTED\n", pat); return NULL;
    }
    struct entry *v = calloc(paths.gl_pathc + 1, sizeof(*v));
    if(!v)
        return LOG_ERRNO("calloc"), NULL;
    for(size_t i = 0; i != paths.gl_pathc; ++i) {
        v[i] = (struct entry){0};
        if(!(v[i].f = open_file(paths.gl_pathv[i], "r")))
            goto err;
        if(!(v[i].path = strdup(paths.gl_pathv[i]))) {
            LOG_ERRNO("strdup");
            goto err;
        }
        if(!read_label(v[i].path, &v[i].label))
            goto err;
    }
    v[paths.gl_pathc] = (struct entry){0};
    globfree(&paths);
    return v;
err:
    globfree(&paths);
    thermal_destroy(v);
    return NULL;
}

bool thermal_destroy(void *d) {
    if(!d)
        return true;
    struct entry *v = d;
    bool ret = true;
    for(struct entry *p = v; p->f; ++p) {
        if(fclose(p->f) == EOF) {
            LOG_ERRNO("fclose");
            ret = false;
        }
        free(p->path);
        free(p->label);
    }
    free(v);
    return ret;
}

bool thermal_update(void *d) {
    puts("thermal");
    for(struct entry *p = d; p->f; ++p) {
        int temp = 0;
        if(!rewind_and_scan(p->f, "%d", &temp))
            return false;
        printf("  %.1f°C %s\n", (double)temp / 1e3, p->label);
    }
    return true;
}

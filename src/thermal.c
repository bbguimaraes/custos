#include "thermal.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

struct data {
    FILE *input;
    char *path;
};

static bool init(struct data *d) {
    return (d->input = open_file(d->path, "r"));
}

void *thermal_init(void) {
    const char *paths[] = {
        "/sys/devices/platform/coretemp.0/hwmon/hwmon5/temp1_input",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon5/temp2_input",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon5/temp3_input",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon5/temp4_input",
        "/sys/devices/platform/coretemp.0/hwmon/hwmon5/temp5_input",
    };
    struct data *v = calloc(ARRAY_SIZE(paths) + 1, sizeof(*v));
    if(!v)
        return LOG_ERRNO("calloc"), NULL;
    for(size_t i = 0; i != ARRAY_SIZE(paths); ++i) {
        if(!(v[i].path = strdup(paths[i]))) {
            LOG_ERRNO("strdup");
            goto err;
        }
        if(!init(v + i))
            goto err;
    }
    return v;
err:
    thermal_destroy(v);
    return NULL;
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
        printf("  %zu: %.1f°C\n", i, (double)temp / 1e3);
    }
    return true;
}

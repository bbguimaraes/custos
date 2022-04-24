#include "thermal.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static const char *PATHS[] = {
    "/sys/devices/platform/coretemp.0/hwmon/hwmon6/temp2_input",
    "/sys/devices/platform/coretemp.0/hwmon/hwmon6/temp3_input",
};

void *thermal_init(void) {
    FILE **v = calloc(ARRAY_SIZE(PATHS), sizeof(*v));
    if(!v)
        return LOG_ERRNO("calloc"), NULL;
    for(size_t i = 0; i != ARRAY_SIZE(PATHS); ++i)
        if(!(v[i] = open_file(PATHS[i], "r")))
            goto err;
    return v;
err:
    thermal_destroy(v);
    return NULL;
}

bool thermal_destroy(void *d) {
    FILE **v = d;
    bool ret = true;
    if(v)
        for(size_t i = 0; i != ARRAY_SIZE(PATHS) && v[i]; ++i)
            if(!close_file(v[i], PATHS[i]))
                ret = false;
    free(v);
    return ret;
}

bool thermal_update(void *d) {
    FILE **v = d;
    puts("thermal");
    for(size_t i = 0; i != ARRAY_SIZE(PATHS); ++i) {
        int temp = 0;
        if(!rewind_and_scan(v[i], "%d", &temp))
            return false;
        printf("  %zu: %.1f°C\n", i, (double)temp / 1e3);
    }
    return true;
}

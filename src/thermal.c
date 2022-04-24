#include "thermal.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static const char *PATHS[] = {
    "/sys/devices/platform/coretemp.0/hwmon/hwmon7/temp2_input",
    "/sys/devices/platform/coretemp.0/hwmon/hwmon7/temp3_input",
};

void *thermal_init(void) {
    FILE **v = calloc(ARRAY_SIZE(PATHS), sizeof(*v));
    if(!v)
        return LOG_ERRNO("calloc"), NULL;
    for(size_t i = 0; i != ARRAY_SIZE(PATHS); ++i)
        if(!(v[i] = fopen(PATHS[i], "r"))) {
            LOG_ERRNO("fopen(\"%s\")", PATHS[i]);
            goto err;
        }
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
            if(fclose(v[i]) == EOF) {
                ret = false;
                LOG_ERRNO("fclose(\"%s\")", PATHS[i]);
            }
    free(v);
    return ret;
}

bool thermal_update(void *d) {
    FILE **v = d;
    puts("thermal");
    for(size_t i = 0; i != ARRAY_SIZE(PATHS); ++i) {
        if(fflush(v[i]) == EOF)
            return LOG_ERRNO("fflush(%s)", PATHS[i]), false;
        rewind(v[i]);
        int temp = 0;
        if(fscanf(v[i], "%d", &temp) != 1) {
            LOG_ERRNO("invalid value read from \"%s\", errno", PATHS[i]);
            return false;
        }
        printf("  %zu: %.1f°C\n", i, (double)temp / 1e3);
    }
    return true;
}

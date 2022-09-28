#include "thermal.h"

#include <errno.h>
#include <glob.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

static const char *PATTERN =
    "/sys/devices/platform/coretemp.0/hwmon/*/temp*_input";

void *thermal_init(void) {
    const char *const pat = PATTERN;
    glob_t paths = {0};
    switch(glob(pat, 0, NULL, &paths)) {
    case GLOB_NOMATCH: LOG_ERR("glob(%s): no files found\n", pat); return NULL;
    case GLOB_NOSPACE: LOG_ERR("glob(%s): NOSPACE\n", pat); return NULL;
    case GLOB_ABORTED: LOG_ERR("glob(%s): ABORTED\n", pat); return NULL;
    }
    FILE **v = calloc(paths.gl_pathc + 1, sizeof(*v));
    if(!v)
        return LOG_ERRNO("calloc"), NULL;
    for(size_t i = 0; i != paths.gl_pathc; ++i)
        if(!(v[i] = open_file(paths.gl_pathv[i], "r")))
            goto err;
    v[paths.gl_pathc] = NULL;
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
    FILE **v = d;
    bool ret = true;
    for(FILE **p = v; *p; ++p)
        if(fclose(*p) == EOF) {
            LOG_ERRNO("fclose");
            ret = false;
        }
    free(v);
    return ret;
}

bool thermal_update(void *d) {
    puts("thermal");
    size_t i = 0;
    for(FILE **p = d; *p; ++p) {
        int temp = 0;
        if(!rewind_and_scan(*p, "%d", &temp))
            return false;
        printf("  %zu: %.1f°C\n", i++, (double)temp / 1e3);
    }
    return true;
}

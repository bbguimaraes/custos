#include "load.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

#define PATH "/proc/loadavg"

void *load_init(struct lua_State *L) {
    (void)L;
    return open_file(PATH, "r");
}

bool load_destroy(void *d) {
    return close_file(d, PATH);
}

bool load_update(void *d, size_t counter) {
    (void)counter;
    FILE *const f = d;
    puts("load");
    if(fflush(f) == EOF)
        return LOG_ERRNO("fflush(" PATH ")"), false;
    rewind(f);
    double l1 = 0, l5 = 0, l15 = 0;
    if(fscanf(f, "%lf %lf %lf", &l1, &l5, &l15) != 3) {
        LOG_ERRNO("invalid content read from \"" PATH "\", errno");
        return false;
    }
    printf("  %.2f %.2f %.2f\n", l1, l5, l15);
    return true;
}

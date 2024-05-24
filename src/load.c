#include "load.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "window.h"

#define PATH "/proc/loadavg"

void *load_init(struct lua_State *L) {
    (void)L;
    return open_file(PATH, "r");
}

bool load_destroy(void *d) {
    return close_file(d, PATH);
}

bool load_update(void *d, size_t counter, struct window *w) {
    (void)counter;
    window_bold_text(w);
    window_print(w, "load\n");
    window_normal_text(w);
    FILE *const f = d;
    if(fflush(f) == EOF)
        return LOG_ERRNO("fflush(" PATH ")"), false;
    rewind(f);
    double l1 = 0, l5 = 0, l15 = 0;
    if(fscanf(f, "%lf %lf %lf", &l1, &l5, &l15) != 3) {
        LOG_ERRNO("invalid content read from \"" PATH "\", errno");
        return false;
    }
    window_printf(w, "  %.2f %.2f %.2f\n", l1, l5, l15);
    return true;
}

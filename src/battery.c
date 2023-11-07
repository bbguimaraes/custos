#include "battery.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define BASE "/sys/class/power_supply/BAT0/"

static const char *const NOW = BASE "energy_now";
static const char *const FULL = BASE "energy_full_design";

struct data {
    FILE *now, *full;
};

void *battery_init(void) {
    struct data *const d = malloc(sizeof(*d));
    if(!d)
        goto e0;
    if(!(d->now = open_file(NOW, "r")))
        goto e1;
    if(!(d->full = open_file(FULL, "r")))
        goto e2;
    return d;
e2:
    close_file(d->now, NOW);
e1:
    free(d);
e0:
    return NULL;
}

bool battery_destroy(void *vd) {
    const struct data *const d = vd;
    bool ret = true;
    ret = close_file(d->now, NOW) && ret;
    ret = close_file(d->full, FULL) && ret;
    return ret;
}

bool battery_update(void *vd) {
    const struct data *const d = vd;
    puts("battery");
    unsigned long now = 0, full = 0;
    if(!rewind_and_scan(d->now, "%lu", &now))
        return false;
    if(!rewind_and_scan(d->full, "%lu", &full))
        return false;
    const float v = 100.0f * (float)now / (float)full;
    printf("  0: %2.2f%% [", v);
    const unsigned n = (unsigned)v / 10;
    for(unsigned i = 0; i != n; ++i)
        putchar('=');
    for(unsigned i = n; i != 10; ++i)
        putchar(' ');
    puts("]");
    return true;
}

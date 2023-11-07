#include "battery.h"

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define NOW "/energy_now"
#define FULL "/energy_full"
#define DESIGN "/energy_full_design"
#define STATUS "/status"

#define STATUS_MAX 32

struct battery {
    char *name, *base;
    FILE *now, *full, *design, *status;
};

static bool init(struct battery *d) {
    char path[CUSTOS_MAX_PATH];
#define X(name, NAME) \
    if(!(join_path(path, 2, d->base, NAME) && (d->name = fopen(path, "r")))) { \
        LOG_ERRNO("fopen(%s" NAME ")", d->base); \
        return false; \
    }
    X(now, NOW)
    X(full, FULL)
    X(design, DESIGN)
    X(status, STATUS)
#undef X
    return true;
}

static bool update(
    struct battery *b,
    unsigned long *now, unsigned long *full, unsigned long *design,
    char status[static STATUS_MAX])
{
    if(!rewind_and_scan(b->now, "%lu", now))
        return false;
    if(!rewind_and_scan(b->full, "%lu", full))
        return false;
    if(!rewind_and_scan(b->design, "%lu", design))
        return false;
    size_t n = STATUS_MAX;
    if(!rewind_and_read(b->status, "status", &n, status))
        return false;
    status[n - 1] = 0;
    return true;
}

static void render(
    const char *name, float charge, float full, const char *status)
{
    printf("  %s: %s ", name, status);
    const float abs = charge / full;
    if(abs == 1.0f)
        fputs("100%", stdout);
    else
        printf("%2.2f%%", 100.0f * abs);
    putchar('|');
    if(charge == 1.0f)
        fputs("100%", stdout);
    else
        printf("%2.2f%%", 100.0f * charge);
    const float WIDTH = 21;
    unsigned int i = 0;
    fputs(" [", stdout);
    for(const unsigned n = (unsigned)(charge * WIDTH); i != n; ++i)
        putchar('=');
    for(const unsigned n = (unsigned)(full * WIDTH); i != n; ++i)
        putchar('-');
    for(char c = '|'; i != WIDTH; ++i, c = '-')
        putchar(c);
    puts("]");
}

void *battery_init(void) {
    struct battery *const v = calloc(2, sizeof(*v));
    if(!v)
        return LOG_ERRNO("callc"), NULL;
    v->name = strdup("0");
    v->base = strdup("/sys/class/power_supply/BAT0");
    if(!init(v)) {
        battery_destroy(v);
        return NULL;
    }
    return v;
}

bool battery_destroy(void *d) {
    bool ret = true;
    for(struct battery *v = d; v->now; ++v) {
#define X(name, NAME) \
    if(v->name && fclose(v->name)) { \
        LOG_ERRNO("fclose(%s" NAME ")", v->base); \
        ret = false; \
    }
        X(now, NOW)
        X(full, FULL)
        X(design, DESIGN)
        X(status, STATUS)
#undef X
        free(v->name);
        free(v->base);
    }
    return ret;
}

bool battery_update(void *d) {
    puts("battery");
    for(struct battery *v = d; v->now; ++v) {
        unsigned long now = 0, full = 0, design = 0;
        char status[STATUS_MAX];
        if(!update(v, &now, &full, &design, status))
            return false;
        render(
            v->name,
            (float)now / (float)design,
            (float)full / (float)design,
            status);
    }
    return true;
}

#include "date.h"

#include <errno.h>
#include <stdio.h>

#include <time.h>

#include "utils.h"

#define DATE_FMT "%Y-%m-%dT%H:%M:%S"
#define DATE_SIZE sizeof("YYYY-MM-DDTHH:MM:SS")

struct tz {
    const char *name, *id;
};

static struct tz tzs[] = {
    {.name = "P ", .id = "US/Pacific"},
    {.name = "E ", .id = "US/Eastern"},
    {.name = "BR", .id = "America/Sao_Paulo"},
    {.name = "CE", .id = "Europe/Prague"},
};

static bool init_tzs(long *v);
static bool restore_tz(const char *tz);

void *date_init(void) {
    long *const v = calloc(ARRAY_SIZE(tzs), sizeof(*v));
    if(!v)
        return LOG_ERRNO("calloc"), NULL;
    const char *const tz = getenv("TZ");
    if(!init_tzs(v) || !restore_tz(tz)) {
        free(v);
        return NULL;
    }
    return v;
}

bool init_tzs(long *v) {
    time_t t = {0};
    if(time(&t) == -1)
        return LOG_ERRNO("time"), false;
    for(size_t i = 0; i != ARRAY_SIZE(tzs); ++i) {
        if(setenv("TZ", tzs[i].id, 1) == -1)
            return LOG_ERRNO("setenv"), false;
        tzset();
        struct tm tm = {0};
        if(!localtime_r(&t, &tm))
            return LOG_ERRNO("localtime_r"), false;
        tm.tm_isdst = 0;
        time_t t_tz = mktime(&tm);
        if(t_tz == -1)
            return LOG_ERRNO("mktime"), false;
        v[i] = t - t_tz + timezone;
    }
    return true;
}

static bool restore_tz(const char *tz) {
    if(tz) {
        if(setenv("TZ", tz, 1) == -1)
            return LOG_ERRNO("setenv"), false;
    } else
        if(unsetenv("TZ") == -1)
            return LOG_ERRNO("unsetenv"), false;
    tzset();
    return true;
}

bool date_destroy(void *d) {
    free(d);
    return true;
}

bool date_update(void *d) {
    const long *const v = d;
    puts("date");
    time_t t = {0};
    if(time(&t) == -1)
        return LOG_ERRNO("time"), false;
    for(size_t i = 0; i != ARRAY_SIZE(tzs); ++i) {
        const time_t t_tz = t - v[i];
        struct tm tm = {0};
        if(!gmtime_r(&t_tz, &tm))
            return LOG_ERRNO("gmtime_r"), false;
        char str[DATE_SIZE] = {0};
        if(strftime(str, sizeof(str), DATE_FMT, &tm) != sizeof(str) - 1)
            return LOG_ERR("strftime failed\n"), false;
        printf("  %s %s\n", tzs[i].name, str);
    }
    return true;
}

#include "custos.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>
#include <signal.h>
#include <time.h>

#include "battery.h"
#include "date.h"
#include "load.h"
#include "mod.h"
#include "term.h"
#include "test.h"
#include "thermal.h"
#include "utils.h"

#define DATE_FMT "%Y-%m-%dT%H:%M:%S"
#define DATE_SIZE sizeof("YYYY-MM-DDTHH:MM:SS")

struct lua_State;

enum flag {
    HELP         = 1 << 0,
    VERBOSE      = 1 << 1,
    CLEAR_SCREEN = 1 << 2,
};

struct config {
    struct timespec t;
    size_t count, interval;
    enum flag flags;
    /** bitmap which corresponds to \ref modules */
    u8 enabled_modules;
    struct lua_State *L;
};

static sig_atomic_t interrupted = 0;

static struct module modules[] = {{
    .name = "battery",
    .init = battery_init,
    .destroy = battery_destroy,
    .update = battery_update,
}, {
    .name = "date",
    .lua = date_lua,
    .init = date_init,
    .destroy = date_destroy,
    .update = date_update,
}, {
    .name = "load",
    .init = load_init,
    .destroy = load_destroy,
    .update = load_update,
}, {
    .name = "test",
    .lua = test_lua,
    .init = test_init,
    .destroy = test_destroy,
    .update = test_update,
}, {
    .name = "thermal",
    .init = thermal_init,
    .destroy = thermal_destroy,
    .update = thermal_update,
}};

static void sigint_handler(int s) {
    (void)s;
    interrupted = 1;
}

static bool parse_enabled(char *s, u8 *ret);

static bool parse_args(int argc, char *const *argv, struct config *config) {
    static const char *short_opts = "hvci:n:m:";
    static const struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"clear", no_argument, 0, 'c'},
        {"count", required_argument, 0, 'n'},
        {"interval", required_argument, 0, 'i'},
        {"modules", required_argument, 0, 'm' },
        {0},
    };
    for(;;) {
        int i = 0;
        const int c = getopt_long(argc, argv, short_opts, long_opts, &i);
        if(c == -1)
            break;
        switch(c) {
        case 'h': config->flags |= HELP; break;
        case 'v': config->flags |= VERBOSE; break;
        case 'c': config->flags |= CLEAR_SCREEN; break;
        case 'n':
            if(!parse_ulong_arg("count", SIZE_MAX, optarg, &config->count))
                return false;
            break;
        case 'i': {
            const size_t max = (time_t)((size_t)-1 / 2);
            if(!parse_ulong_arg("interval", max, optarg, &config->interval))
                return false;
            break;
        }
        case 'm':
            if(!parse_enabled(optarg, &config->enabled_modules))
                return false;
            break;
        default: return false;
        }
    }
    return true;
}

static void usage(FILE *f, const char *name) {
    fprintf(f,
        "Usage: %s [OPTIONS]\n"
        "\n"
        "Options:\n"
        "    -h, --help             this help\n"
        "    -v, --verbose          verbose output\n"
        "    -c, --clear            clear screen between updates\n"
        "    -n N, --count N        exit after N updates\n"
        "    --modules LIST...      load only modules in LIST, a\n"
        "                           comma-separated list of names\n",
        name);
}

static bool parse_enabled(char *s, u8 *ret) {
    u8 enabled = 0;
    char *save = NULL, *p = strtok_r(s, ",", &save);
    if(!p)
        return LOG_ERR("invalid module list: %s\n", s), false;
    do {
        const struct module key = {.name = p};
        const struct module *const m = bsearch(
            &key, modules, ARRAY_SIZE(modules), sizeof(*modules),
            module_name_cmp);
        if(!m)
            return LOG_ERR("invalid module name: %s\n", key.name), false;
        enabled |= (u8)(1u << (m - modules));
    } while((p = strtok_r(NULL, ",", &save)));
    if(enabled)
        *ret = enabled;
    return true;
}

static bool print_header(const struct config *config) {
    time_t t = {0};
    if(time(&t) == -1)
        return LOG_ERRNO("time"), false;
    struct tm tm = {0};
    if(!localtime_r(&t, &tm))
        return LOG_ERRNO("localtime_r"), false;
    char str[DATE_SIZE] = {0};
    if(strftime(str, sizeof(str), DATE_FMT, &tm) != sizeof(str) - 1)
        return LOG_ERR("strftime failed\n"), false;
    if(config->flags & CLEAR_SCREEN)
        term_clear(stdout);
    else
        puts("");
    printf("%s\n\n", str);
    return true;
}

static bool sleep(struct config *config) {
    struct timespec *t = &config->t;
    t->tv_sec = t->tv_sec + (time_t)config->interval;
    while(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, t, NULL) == -1)
        if(errno != EINTR)
            return LOG_ERRNO("clock_nanosleep"), false;
    return true;
}

static bool init_config(struct config *c) {
    struct sigaction sa = {.sa_handler = sigint_handler};
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGINT, &sa, NULL) == -1)
        return LOG_ERRNO("signaction"), false;
    if(clock_gettime(CLOCK_MONOTONIC, &c->t) == -1)
        return LOG_ERRNO("clock_gettime"), false;
    return true;
}

static bool init_modules(struct config *config) {
    struct lua_State *const L = config->L;
    const u8 enabled = config->enabled_modules;
    for(size_t i = 0; i != ARRAY_SIZE(modules); ++i) {
        struct module *const m = modules + i;
        if(enabled && !(enabled & (1u << i)))
            continue;
        if(!(m->data = m->init(L))) {
            LOG_ERR("failed to initialize module \"%s\"\n", m->name);
            return false;
        }
    }
    return true;
}

static bool destroy_modules(void);

static bool destroy(struct config *c) {
    bool ret = true;
    ret = destroy_modules() && ret;
    if(c->L)
        custos_lua_destroy(c->L);
    return ret;
}

static bool destroy_modules(void) {
    bool ret = true;
    for(size_t i = 0; i != ARRAY_SIZE(modules); ++i) {
        struct module *const m = modules + i;
        if(!m->data)
            continue;
        if(!m->destroy(m->data)) {
            LOG_ERR("failed to destroy module \"%s\"\n", m->name);
            ret = false;
        }
    }
    return ret;
}

static bool update_modules(void) {
    for(size_t i = 0; i != ARRAY_SIZE(modules); ++i) {
        struct module *const m = modules + i;
        if(!m->data)
            continue;
        if(!m->update(m->data)) {
            LOG_ERR("failed to update module \"%s\"\n", m->name);
            return false;
        }
    }
    return true;
}

struct module *get_modules(size_t *n) {
    *n = ARRAY_SIZE(modules);
    return modules;
}

int main(int argc, char *const *argv) {
    struct config config = {
        .interval = 1,
    };
    if(!parse_args(argc, argv, &config))
        return 1;
    if(config.flags & HELP)
        return usage(stdout, argv[0]), 0;
    if(!(init_config(&config)
        && (config.L = custos_lua_init())
        && custos_load_config(config.L, &config.enabled_modules)
        && init_modules(&config)
    ))
        return 1;
    while(!interrupted) {
        if(!(print_header(&config) && update_modules()))
            goto err;
        if(config.count && !--config.count)
            break;
        if(!sleep(&config))
            goto err;
    }
    return !destroy(&config);
err:
    destroy(&config);
    return 1;
}

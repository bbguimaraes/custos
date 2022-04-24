#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <getopt.h>
#include <signal.h>
#include <time.h>

#include "test.h"
#include "utils.h"

#define DATE_FMT "%Y-%m-%dT%H:%M:%S"
#define DATE_SIZE sizeof("YYYY-MM-DDTHH:MM:SS")

enum flag {
    HELP    = 1 << 0,
    VERBOSE = 1 << 1,
};

struct config {
    struct timespec t;
    enum flag flags;
};

struct module {
    const char *name;
    void *data;
    void *(*init)(void);
    bool (*destroy)(void*);
    bool (*update)(void*);
};

static sig_atomic_t interrupted = 0;

static struct module modules[] = {{
    .name = "test",
    .init = test_init,
    .destroy = test_destroy,
    .update = test_update,
}};

static void sigint_handler(int s) {
    (void)s;
    interrupted = 1;
}

static bool parse_args(int argc, char *const *argv, struct config *config) {
    static const char *short_opts = "hv";
    static const struct option long_opts[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
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
        "    -v, --verbose          verbose output\n",
        name);
}

static bool print_header(void) {
    time_t t = {0};
    if(time(&t) == -1)
        return LOG_ERRNO("time"), false;
    struct tm tm = {0};
    if(!localtime_r(&t, &tm))
        return LOG_ERRNO("localtime_r"), false;
    char str[DATE_SIZE] = {0};
    if(strftime(str, sizeof(str), DATE_FMT, &tm) != sizeof(str) - 1)
        return LOG_ERR("strftime failed\n"), false;
    printf("\n%s\n\n", str);
    return true;
}

static bool sleep(struct config *config) {
    struct timespec *t = &config->t;
    ++t->tv_sec;
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

static bool init_modules(void) {
    for(size_t i = 0; i != ARRAY_SIZE(modules); ++i) {
        struct module *const m = modules + i;
        if(!(m->data = m->init())) {
            LOG_ERR("failed to initialize module \"%s\"\n", m->name);
            return false;
        }
    }
    return true;
}

static bool destroy_modules(void) {
    bool ret = true;
    for(size_t i = 0; i != ARRAY_SIZE(modules); ++i) {
        struct module *const m = modules + i;
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
        if(!m->update(m->data)) {
            LOG_ERR("failed to update module \"%s\"\n", m->name);
            return false;
        }
    }
    return true;
}

int main(int argc, char *const *argv) {
    struct config config = {0};
    if(!parse_args(argc, argv, &config))
        return 1;
    if(config.flags & HELP)
        return usage(stdout, argv[0]), 0;
    if(!(init_config(&config) && init_modules()))
        return 1;
    while(!interrupted)
        if(!(print_header() && update_modules() && sleep(&config)))
            goto err;
    return !destroy_modules();
err:
    destroy_modules();
    return 1;
}

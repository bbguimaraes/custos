#ifndef CUSTOS_MOD_H
#define CUSTOS_MOD_H

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

struct module {
    const char *name;
    void *data;
    void *(*init)(void);
    bool (*destroy)(void*);
    bool (*update)(void*);
};

struct module *get_modules(size_t *n);
static inline int module_name_cmp(const void *lhs, const void *rhs);

inline int module_name_cmp(const void *lhs, const void *rhs) {
    return strcmp(((struct module*)lhs)->name, ((struct module*)rhs)->name);
}

#endif
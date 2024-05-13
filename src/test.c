#include "test.h"

#include <errno.h>
#include <stdio.h>

#include <lauxlib.h>

void test_lua(struct lua_State *L) {
    lua_getglobal(L, "print");
    lua_pushstring(L, __func__);
    lua_call(L, 1, 0);
}

void *test_init(struct lua_State *L) {
    puts(__func__);
    printf("    L: %p\n", (void*)L);
    return &errno;
}

bool test_destroy(void *d) {
    (void)d;
    puts(__func__);
    return true;
}

bool test_update(void *d, size_t counter) {
    (void)d;
    printf("%s %zu\n", __func__, counter);
    return true;
}

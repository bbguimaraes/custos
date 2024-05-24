#ifndef CUSTOS_TEST_H
#define CUSTOS_TEST_H

#include <stdbool.h>
#include <stddef.h>

struct lua_State;

struct window;

void test_lua(struct lua_State *L);
void *test_init(struct lua_State *L);
bool test_destroy(void *d);
bool test_update(void *d, size_t counter, struct window *w);

#endif

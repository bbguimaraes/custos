#include "test.h"

#include <errno.h>
#include <stdio.h>

void *test_init(void) {
    puts(__func__);
    return &errno;
}

bool test_destroy(void *d) {
    (void)d;
    puts(__func__);
    return true;
}

bool test_update(void *d) {
    (void)d;
    puts(__func__);
    return true;
}

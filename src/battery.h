#ifndef CUSTOS_BATTERY_H
#define CUSTOS_BATTERY_H

#include <stdbool.h>

void *battery_init(void);
bool battery_destroy(void *d);
bool battery_update(void *d);

#endif

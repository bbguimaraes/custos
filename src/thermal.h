#ifndef CUSTOS_THERMAL_H
#define CUSTOS_THERMAL_H

#include <stdbool.h>

void *thermal_init(void);
bool thermal_destroy(void *d);
bool thermal_update(void *d);

#endif

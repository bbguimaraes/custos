#ifndef CUSTOS_DATE_H
#define CUSTOS_DATE_H

#include <stdbool.h>

void *date_init(void);
bool date_destroy(void *d);
bool date_update(void *d);

#endif

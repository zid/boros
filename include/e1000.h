#ifndef E1000_H
#define E1000_H
#include "types.h"
#include "device.h"
void e1000_init(struct device *dev);
void e1000_send(uintptr_t buf, size_t len);
#endif

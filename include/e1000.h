#ifndef E1000_H
#define E1000_H
#include "types.h"
void e1000_init(u8 bus, u8 dev);
void e1000_send(uintptr_t buf, size_t len);
#endif

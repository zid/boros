#ifndef CPU_H
#define CPU_H
#include "types.h"
unsigned long get_cr3(void);
void set_cr3(u64);
void out32(u16, u32);
u32 in32(u16);
#endif

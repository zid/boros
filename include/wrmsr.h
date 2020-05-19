#ifndef WRMSR_H
#define WRMSR_H
#include <types.h>
void wrmsr(u32 addr, u64 value);
#endif

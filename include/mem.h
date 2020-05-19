#ifndef MEM_H
#define MEM_H
#include "types.h"

#define PT_PRESENT 1
#define PT_WR      2

enum PAGE_SIZE {
	PAGE_2M,
	PAGE_4K
};

void mem_init(void *);
void mmap(u64 vaddr, u64 paddr, u64 len, u32 flags);
u64  phys_alloc(enum PAGE_SIZE);
void *phys_to_virt(uintptr_t paddr);
uintptr_t virt_to_phys(void *vaddr);
void *kalloc(void);
void kfree(void *);
#endif

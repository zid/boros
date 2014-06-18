#include "mem.h"
#include "cpu.h"
#include "print.h"

unsigned long free_page;

struct page_table {
	unsigned long addr;
};

static unsigned long phys_alloc()
{
	unsigned long p;
	p = free_page;
	free_page += 0x1000;
	return p;
}

static void map(unsigned long vaddr, unsigned long paddr)
{
	unsigned long *p;
	unsigned long pml4e, pdpte, pde, pte;

	pml4e = (vaddr>>39)&0x1FF;
	pdpte = (vaddr>>30)&0x1FF;
	pde = (vaddr>>21)&0x1FF;
	pte = (vaddr>>12)&0x1FF;
	asm("xchg %bx, %bx");
	p = (unsigned long *)0xFFFFFF7FBFDFE000ULL + pml4e;
	if(!*p)
	{
		*p = phys_alloc() | 3;
		set_cr3(get_cr3());
	}

	p = (unsigned long *)(0xFFFFFF7FBFC00000ULL + (pml4e * 0x1000)) + pdpte;
	if(!*p)
	{
		*p = phys_alloc() | 3;
		set_cr3(get_cr3());
	}

	p = (unsigned long *)(0xFFFFFF7F80000000 + (pml4e * 0x200000) + (pdpte*0x1000)) + pde;
	if(!*p)
	{
		*p = phys_alloc() | 3;
		set_cr3(get_cr3());
	}

	p = (unsigned long *)(0xFFFFFF0000000000 + (pml4e * 0x40000000) + (pdpte*0x200000) + (pde*0x1000)) + pte;
	if(!*p)
	{
		*p = paddr | 3;
		set_cr3(get_cr3());
	}

}

void init_mem(unsigned long freeptr, void *b)
{
	free_page = freeptr;

	map(0xB8000, 0xB8000);
	print("%lx\n", free_page);
}

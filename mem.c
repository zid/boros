#include "mem.h"
#include "cpu.h"
#include "print.h"

#define RECURSE 510UL
#define RECURSE_PML4 (0xFFFF000000000000UL | (RECURSE<<39))

unsigned long free_page;

static unsigned long *page_table(unsigned long p3, unsigned long p2, unsigned long p1)
{
	return RECURSE_PML4 | (p3<<30) | (p2<<21) | (p1<<12);
}

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
	pde   = (vaddr>>21)&0x1FF;
	pte   = (vaddr>>12)&0x1FF;

	/* Do we need to make a new PDPT? */
	p = page_table(RECURSE, RECURSE, RECURSE) + pml4e;
	if(!*p)
	{
		*p = phys_alloc() | 3;
		set_cr3(get_cr3());
	}

	/* Do we need to make a new PD? */
	p = page_table(RECURSE, RECURSE, pml4e) + pdpte;
	if(!*p)
	{
		*p = phys_alloc() | 3;
		set_cr3(get_cr3());
	}

	/* Do we need to make a new PT? */
	p = page_table(RECURSE, pml4e, pdpte) + pde;
	if(!*p)
	{
		*p = phys_alloc() | 3;
		set_cr3(get_cr3());
	}

	/* Map the page into the page table */
	p = page_table(pml4e, pdpte, pde) + pte;
	*p = paddr | 3;
	set_cr3(get_cr3());
}

void init_mem(unsigned long freeptr, void *b)
{
	free_page = freeptr;

	map(0xB8000, 0xB8000);
	print("%lx\n", free_page);
}

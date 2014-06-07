#include "mem.h"
#include "cpu.h"

static void *free_ptr;

/* Which 512GB region do we cover? 0-511 */
unsigned long pml4_get_index(unsigned long addr)
{
	return ((addr & 0xFF8000000000UL)>>39);
}

/* Which 1GB region do we cover? 0-511 */
unsigned long pdpt_get_index(unsigned long addr)
{
	return ((addr & 0x7FC0000000UL)>>30);
}

unsigned long pd_get_index(unsigned long addr)
{
	return ((addr & 0x3FE00000UL)>>21);
}

unsigned long pt_get_index(unsigned long addr)
{
	return ((addr & 0x1FF000UL)>>12);
}

unsigned long get_entry(unsigned long table, unsigned long index)
{
	unsigned long entry;
	unsigned int *addr;

	addr = (unsigned int *)(table + (index<<3));

	entry = *(addr+1);
	entry = entry<<32;
	entry |= *addr;

	return entry;
}

void set_entry(unsigned long table, unsigned long index, unsigned long entry)
{
	*((unsigned long *)table + (index<<3)) = entry | 3;
}

/* The bootstrap is mapped to 1MB so there will be a PDPT and PD already */
void map_text_console()
{
	unsigned long pml4, pdpt, pd, pt;
	unsigned long pml4_index, pdpt_index, pd_index, pt_index;
	unsigned long text_console_addr = 0xB8000, phys_addr;

	pml4 = get_cr3();
	pml4_index = pml4_get_index(0xB8000);
	pdpt = get_entry(pml4, pml4_index) & ~0xFFF;
	pdpt_index = pdpt_get_index(text_console_addr);
	pd = get_entry(pdpt, pdpt_index) & ~0xFFF;
	pd_index = pd_get_index(text_console_addr);
	pt = get_entry(pd, pd_index) & ~0xFFF;
	pt_index = pt_get_index(text_console_addr);
	phys_addr = get_entry(pt, pt_index) & ~0xFFF;
	if(!phys_addr)
		set_entry(pt, pt_index, 0xB8000 /* identity map */);
}

void init_mem(void *free_page, void *e820)
{
	free_ptr = free_page;

	map_text_console();
}

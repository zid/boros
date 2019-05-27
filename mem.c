#include "mem.h"
#include "cpu.h"
#include "print.h"

#define RECURSE 510UL
#define RECURSE_PML4 (0xFFFFFF0000000000UL)
#define VIRT_TO_PHYS (0xFFFFFF8000000000UL)
#define BITMAP_BASE  (0xFFFFFE8000000000UL)

#define TWO_MEGS (2*1024*1024)

struct e820_entry {
	u32 size;
	u64 addr;
	u64 len;
	u32 type;
} __attribute__((packed));

struct mem_info {
	/* Next free phys. page for early alloc */
	u32 free_page;
	/* Amount of e820 entries */
	u32 e820_len;
	/* E820 contents */
	struct e820_entry e[];
};

struct node {
	struct node *next;
};

static struct node *freelist[2];
static u64 free_page;

u64 phys_to_virt(u64 paddr)
{
	return paddr+VIRT_TO_PHYS;
}

static u64 *page_table(unsigned long p3, unsigned long p2, unsigned long p1)
{
	return (unsigned long *)(RECURSE_PML4 | (p3<<30) | (p2<<21) | (p1<<12));
}

u64 early_phys_alloc(void)
{
	u64 page;

	page = free_page;
	free_page += 4096;
	return page;
}

void early_map(unsigned long vaddr, unsigned long paddr)
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
		*p = early_phys_alloc() | PT_WR | PT_PRESENT;

	/* Do we need to make a new PD? */
	p = page_table(RECURSE, RECURSE, pml4e) + pdpte;
	if(!*p)
		*p = early_phys_alloc() | PT_WR | PT_PRESENT;

	/* Do we need to make a new PT? */
	p = page_table(RECURSE, pml4e, pdpte) + pde;
	if(!*p)
		*p = early_phys_alloc() | PT_WR | PT_PRESENT;

	/* Map the page into the page table */
	p = page_table(pml4e, pdpte, pde) + pte;
	*p = paddr | PT_WR | PT_PRESENT;
}

static int is_free(u64 page)
{
	u64 bit_offset;
	unsigned char *b;

	bit_offset = page/4096;
	b = (unsigned char *)(BITMAP_BASE + bit_offset/8);
	return *b & 1<<(bit_offset%8);
}

static void bitmap_free_page(u64 page)
{
	u64 bit_offset;
	unsigned char *b;

	bit_offset = page/4096;
	b = (unsigned char *)(BITMAP_BASE + bit_offset/8);
	*b |= 1U<<(bit_offset%8);
}

static void bitmap_claim_page(u64 page)
{
	u64 bit_offset;
	unsigned char *b;

	bit_offset = page/4096;
	b = (unsigned char *)(BITMAP_BASE + bit_offset/8);
	*b &= ~(1U<<(bit_offset%8));
}

/* Every 128MB of memory needs 1 bitmap page */
static void bitmap_add_range(const u64 addr, const u64 len)
{
	u64 page, offset;
	const u64 ratio = (128*1024*1024UL);

	/* Walk forwards in 128MB increments after rounding down */
	offset = addr & ~(ratio-1);
	for(; offset < addr + len; offset += ratio)
		early_map(BITMAP_BASE + offset/32768, early_phys_alloc());

	/* Initialize them to free */
	for(page = addr; page < addr + len - 4096; page += 4096)
	{
		/* Unless we already used it */
		if(page >= 0x100000 && page < free_page)
			bitmap_claim_page(page);
		else
			bitmap_free_page(page);
	}
}

static void phys_add_page(u64 page, enum PAGE_SIZE size)
{
	struct node *n;
	struct node *t;

	/* Don't add pages already used by early_phys_alloc */
	if(!is_free(page))
		return;

	t = (void *)phys_to_virt(page);
	n = freelist[size];

	freelist[size] = (struct node *)page;
	t->next = n;
}

static void phys_add_range(u64 addr, u64 len)
{
	u64 page;

	/* Allocate bitmap pages if needed, and mark the pages as free */
	bitmap_add_range(addr, len);

	/* Note: Has integer under/overflow issues */
	for(page = addr; page < addr+len-TWO_MEGS; page += TWO_MEGS)
		phys_add_page(page, PAGE_2M);
	for(; page < addr+len; page += 4096)
		phys_add_page(page, PAGE_4K);

}

static void phys_fix_early_gap(void)
{
	u64 page;
	u64 aligned;

	aligned = ((free_page+TWO_MEGS-1)/TWO_MEGS)*TWO_MEGS;
	if(aligned == free_page)
		return;

	for(page = free_page; page < aligned; page += 4096)
		phys_add_page(page, PAGE_4K);
}

u64 phys_alloc(enum PAGE_SIZE size)
{
	u64 n;

	n = (u64)freelist[size];
	if(!n)
		return 0;

	freelist[size] = ((struct node *)phys_to_virt(n))->next;
	/* TODO: bitmap */

	/* Clean up the freelist pointer */
	*((u64 *)phys_to_virt(n)) = 0;
	return n;
}

void mmap(u64 vaddr, u64 paddr, u64 len, u32 flags)
{
	u64 *p;
	u64 pml4e, pdpte, pde, pte, v;

	for(v = vaddr; v < vaddr+len; v += 4096, paddr += 4096)
	{
		pml4e = (v>>39)&0x1FF;
		pdpte = (v>>30)&0x1FF;
		pde   = (v>>21)&0x1FF;
		pte   = (v>>12)&0x1FF;

		/* Do we need to make a new PDPT? */
		p = page_table(RECURSE, RECURSE, RECURSE) + pml4e;
		if(!*p)
			*p = phys_alloc(PAGE_4K) | flags | PT_PRESENT;

		/* Do we need to make a new PD? */
		p = page_table(RECURSE, RECURSE, pml4e) + pdpte;
		if(!*p)
			*p = phys_alloc(PAGE_4K) | flags | PT_PRESENT;

		/* Do we need to make a new PT? */
		p = page_table(RECURSE, pml4e, pdpte) + pde;
		if(!*p)
			*p = phys_alloc(PAGE_4K) | flags | PT_PRESENT;

		/* Map the page into the page table */
		p = page_table(pml4e, pdpte, pde) + pte;

		/* Entry already exists, don't map it twice */
#if 0
		if(*p)
			{printf("mmap: Already mapped, skipping: %lx %lx\n", p, *p);continue;}
#endif
		*p = paddr | flags | PT_PRESENT;
	}
}

void mem_init(void *m)
{
	struct mem_info *mem = m;
	struct e820_entry *e;

	free_page = mem->free_page;

	early_map(0xB8000, 0xB8000);

	for(e = mem->e; e < &mem->e[mem->e820_len]; e++)
	{
		if(e->type != 1)
			continue;
		if(e->addr < 0x100000)
			continue;
		phys_add_range(e->addr, e->len);
	}

	/* We might have a gap between the end of the kernel and the next
	 * 2M boundary, add those pages to the list too. */
	phys_fix_early_gap();
}


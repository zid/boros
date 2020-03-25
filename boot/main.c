#include "types.h"
#include "multiboot.h"
#include "elf.h"
#include "print.h"

#define VIRT_TO_PHYS (0xFFFFFF8000000000UL)	/* PML4 511 */

#define TWO_MEGS (2*1024*1024)

extern u32 _bootloader_size;
extern u32 _stack_start;

void __attribute__((noreturn)) go_long(u32, u64, u32);

struct page_table_entry
{
	union
	{
		struct
		{
			u64 present :1;
			u64 rw      :1;
			u64 us      :1;
			u64 pwt     :1;
			u64 pcd     :1;
			u64 a       :1;
			u64 dirty   :1;
			u64 pat     :1;
			u64 global  :1;
			u64 ignore2 :3;
			u64 addr    :40;
			u64 ignore3 :7;
			u64 pke     :4;
			u64 nx      :1;
		};
		u64 entry;
	};
};

struct page_table
{
	struct page_table_entry e[512];
};

struct mem_info
{
	unsigned long free;
	unsigned long e820_count;
	char e820_buf[480];
} mem;

static unsigned long free_page;
static struct page_table *pml4;

static void clear_page(u32 page)
{
	char *p;

	for(p = (char *)page; p < (char *)page+4096; p++)
		*p = 0;
}

static unsigned long new_page(void)
{
	unsigned long t;

	t = free_page;
	free_page += 4096;
	clear_page(t);

	return t;
}

static void map_page(u64 vaddr, u32 paddr, u32 flags, u32 is_kernel)
{
	int pml4e, pdpte, pde, pte;
	struct page_table *p;
	u32 new;

	/* Calculate which entry in each table this address corresponds to */
	pml4e = (vaddr>>39) & 0x1FF;
	pdpte = (vaddr>>30) & 0x1FF;
	pde   = (vaddr>>21) & 0x1FF;
	pte   = (vaddr>>12) & 0x1FF;

	/* If we haven't made set of page tables yet, make some */
	if(!pml4)
		pml4 = (struct page_table *)new_page(); /* New PML4 */

	p = pml4;

	if(!p->e[pml4e].present)
	{
		new = new_page(); /* New PDPT */
		pml4->e[pml4e].entry = new;
		pml4->e[pml4e].rw = 1;
		pml4->e[pml4e].us = 1;
		pml4->e[pml4e].present = 1;
		p = (struct page_table *)new;
	} else {
		p = (struct page_table *)(u32)(p->e[pml4e].entry & 0xFFFFFFFFFE00ULL);
	}

	if(!p->e[pdpte].present)
	{
		new = new_page(); /* New PD */
		p->e[pdpte].entry = new;
		p->e[pdpte].rw = 1;
		p->e[pdpte].us = 1;
		p->e[pdpte].present = 1;
		p = (struct page_table *)new;
	} else {
		p = (struct page_table *)(u32)(p->e[pdpte].entry & 0xFFFFFFFFFE00ULL);
	}

	if(!p->e[pde].present)
	{
		new = new_page(); /* New PT */
		p->e[pde].entry = new;
		p->e[pde].rw = 1;
		p->e[pde].us = 1;
		p->e[pde].present = 1;
		p = (struct page_table *)new;
	} else {
		p = (struct page_table *)(u32)(p->e[pde].entry & 0xFFFFFFFFFE00ULL);
	}
	p->e[pte].entry = paddr;
	p->e[pte].rw = !!(flags & PF_W);
	p->e[pte].nx = !(flags & PF_X);
	p->e[pte].us = !is_kernel;
	p->e[pte].present = 1;
}


static void map_section(struct program_header *p, u64 elf_start, u32 is_kernel)
{
	u64 paddr, vaddr;

	paddr = p->p_offset + elf_start;
	printf("Map section: %lx %lx\n", p->p_vaddr, paddr);
	/* Map each page in the section */
	for(vaddr = p->p_vaddr; vaddr < p->p_vaddr + p->p_memsz; vaddr += 4096)
	{
		if(p->p_filesz)
		{
			/* This page is backed by the file, subtract 4096 or
			 * filesz bytes, whichever is smaller. */
			p->p_filesz -= p->p_filesz >= 4096 ? 4096 : p->p_filesz;
		}
		else
		{
			/* We ran out of file bytes, this must be bss,
			 * allocate memory and zero it instead. */
			paddr = new_page();
		}
		printf("Mapping %lx to %lx (%x)\n", vaddr, paddr, p->p_flags);
		map_page(vaddr, paddr, p->p_flags, is_kernel);
		paddr += 4096;

	}
}

static void map_bootloader(u32 size)
{
	u32 addr, end;

	/* Map the pages before .bss as executable */
	end = (u32)&_stack_start - 0x1000;
	for(addr = 0x100000; addr < end; addr += 0x1000)
		map_page(addr, addr, PF_X, 1);

	/* Map the pages in .bss as writeable */
	printf("Addr: %x Size: %x\n", addr, size);
	for(addr; addr < end + size; addr += 0x1000)
		map_page(addr, addr, PF_W, 1);
}

static void map_range(unsigned long start, unsigned long len)
{
	unsigned long page;

	/* Displaced identity map each page */
	for(page = start; page < start + len; page += 0x1000)
		map_page(VIRT_TO_PHYS + page, page, PF_W, 0);
}

static void memcpy(void *d, void *s, unsigned long n)
{
	unsigned char *dd = d, *ss = s;
	while(n--)
		*dd++ = *ss++;
}

static void init_mem(struct multiboot *mb, unsigned long kernel_end)
{
	struct mem *m;

	free_page = kernel_end;

	for(m = mb->mmap_addr; (char *)m < (char *)mb->mmap_addr + mb->mmap_len; m++)
	{
		if(m->type != 1)
			continue;
		if(m->addr < 0x100000)
			continue;
		if(m->addr >= 0x100000000UL)
			continue;

		map_range(m->addr, m->addr + m->len);
	}

	if(mb->mmap_len > 480)
		printf("E820 too big for buffer!\n");

	/* Save a copy of the e820 for the kernel */
	memcpy(mem.e820_buf, mb->mmap_addr, mb->mmap_len);
	mem.e820_count  = mb->mmap_len / 24;
}

void __attribute__((noreturn)) main(struct multiboot *mb)
{
	int i;
	u32 kernel_start, kernel_end;
	struct elf_header *e, *ke;
	struct program_header *p;
	struct module *mod = mb->mods_addr, *last_mod;

	printf("Bootloader started\n");

	if(mb->mods_count < 1)
	{
		printf("Fatal: Kernel not supplied as grub module.\n");
		while(1);
	}

	kernel_start = mod->mod_start;

	last_mod = &mod[mb->mods_count-1];
	kernel_end = ((last_mod->mod_end + 4095)/4096)*4096;

	printf("Kernel: %x-%x\n", kernel_start, kernel_end);

	/* Map RAM below 4G for the kernel to start with */
	init_mem(mb, kernel_end);

	printf("Memory initialized\n");

	ke = (struct elf_header *)kernel_start;
	p = (struct program_header *)(u32)(ke->e_phoff + kernel_start);

	for(i = 0; i < ke->e_phnum; i++)
		map_section(&p[i], kernel_start, 1);

	printf("Kernel mapped\n");

	map_bootloader((u32)&_bootloader_size);
	printf("Bootloader mapped\n");

	while(mb->mods_count-- > 1)
	{
		printf("Mapping module\n");
		mod++;
		e = (struct elf_header *)mod->mod_start;
		printf("e->e_phoff: %x\n", e->e_phoff);
		p = (struct program_header *)(u32)(e->e_phoff + (u32)e);
		printf("e: %x, p: %x\n", e, p);
		for(i = 0; i < e->e_phnum; i++)
			map_section(&p[i], (u32)e, 0);
	}

	printf("Going to long mode...\n");

	mem.free = free_page;

	go_long((u32)pml4, ke->e_entry, (u32)&mem);
}

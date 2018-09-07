#include "types.h"
#include "multiboot.h"
#include "elf.h"
#include "print.h"

extern u32 _bootloader_size;

void __attribute__((noreturn)) go_long(u32, u64, u32);

struct page_table_entry
{
	union {
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
		u64 entry;
	};
};

struct page_table
{
	struct page_table_entry e[512];
};

static struct memory
{
	u32 free_page;
	u64 elf_start;
	struct page_table *pml4;
} mem;

void clear_page(u32 page)
{
	char *p;

	for(p = (char *)page; p < (char *)page+4096; p++)
		*p = 0;
}

u32 new_page(void)
{
	u32 n = mem.free_page;
	printf("Allocated: %x\n", n);
	mem.free_page = *((u32 *)mem.free_page);
	clear_page(n);

	return n;
}

void map_page(u64 vaddr, u32 paddr, u32 flags)
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
	if(!mem.pml4)
		mem.pml4 = (struct page_table *)new_page(); /* New PML4 */

	p = mem.pml4;

	if(!p->e[pml4e].present)
	{
		new = new_page(); /* New PDPT */
		mem.pml4->e[pml4e].entry = new;
		mem.pml4->e[pml4e].present = 1;
		p = (struct page_table *)new;
	} else {
		p = (struct page_table *)(u32)(p->e[pml4e].entry & 0xFFFFFFFFFE00ULL);
	}

	if(!p->e[pdpte].present)
	{
		new = new_page(); /* New PD */
		p->e[pdpte].entry = new;
		p->e[pdpte].present = 1;
		p = (struct page_table *)new;
	} else {
		p = (struct page_table *)(u32)(p->e[pdpte].entry & 0xFFFFFFFFFE00ULL);
	}

	if(!p->e[pde].present)
	{
		new = new_page(); /* New PT */
		p->e[pde].entry = new;
		p->e[pde].present = 1;
		p = (struct page_table *)new;
	} else {
		p = (struct page_table *)(u32)(p->e[pde].entry & 0xFFFFFFFFFE00ULL);
	}

	p->e[pte].entry = paddr;
	p->e[pte].rw = !!(flags & SHF_WRITE);
	p->e[pte].nx = !(flags & SHF_EXECINSTR);
	p->e[pte].present = 1;
}


void map_section(struct program_header *p)
{
	u64 paddr, vaddr;

	paddr = p->p_offset + mem.elf_start;
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
		printf("Mapping %lx to %lx\n", vaddr, paddr);
		map_page(vaddr, paddr, p->p_flags);
		paddr += 4096;

	}
}

void map_bootloader(u32 size)
{
	u32 addr;

	for(addr = 0x100000; addr < 0x100000 + size; addr += 4096)
	{
		map_page(addr, addr, SHF_EXECINSTR);
	}
}

static void page_add(unsigned long page)
{
	unsigned long *pp;

	pp = (unsigned long *)page;
	pp[0] = mem.free_page;
	pp[1] = 0;
	mem.free_page = page;
}

static int is_kernel_page(unsigned long kernel_size, unsigned long page)
{
	if(page >= 0x100000 && page < 0x100000 + kernel_size)
		return 1;
	return 0;
}

static void init_mem(struct multiboot *mb, unsigned long kernel_end)
{
	struct mem *m;

	for(m = mb->mmap_addr; (char *)m < (char *)mb->mmap_addr + mb->mmap_len; m++)
	{
		unsigned long page, start, end;

		/* RAM */
		if(m->type != 1)
			continue;
		if(m->addr < 0x100000)
			continue;
		if(m->addr >= 0x100000000UL)
			continue;

		start = m->addr;
		end   = start + m->len;

		for(page = start; page < end; page += 4096)
		{
			if(is_kernel_page(kernel_end, page))
				continue;

			page_add(page);
		}
	}
}

void __attribute__((noreturn)) main(struct multiboot *mb)
{
	int i;
	u32 kernel_start, kernel_size, kernel_end;
	struct elf_header *e;
	struct program_header *p;
	struct module *mod = mb->mods_addr;

	printf("Bootloader started\n");

	kernel_start = mod->mod_start;
	kernel_size  = mod->mod_end - mod->mod_start;
	kernel_size  = ((kernel_size+4095)/4096)*4096;
	kernel_end   = kernel_start + kernel_size;

	printf("Kernel: %lx-%lx\n", kernel_start, kernel_end);

	init_mem(mb, kernel_end);

	printf("Memory initialized\n");

	mem.elf_start = kernel_start;
	mem.pml4 = 0;

	e = (struct elf_header *)kernel_start;
	p = (struct program_header *)(u32)(e->e_phoff + kernel_start);

	for(i = 0; i < e->e_phnum; i++)
		map_section(&p[i]);

	printf("Kernel mapped\n");

	map_bootloader((u32)&_bootloader_size);

	printf("Bootloader mapped\n");
	printf("Going to long mode...\n");

	go_long((u32)mem.pml4, e->e_entry, mem.free_page);
}

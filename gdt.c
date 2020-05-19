#include <types.h>
#include <stdlib.h>

struct gdt_entry
{
	u64 limit_low  :16;
	u64 base_low   :24;
	u64 flags_low  :8;
	u64 limit_high :4;
	u64 flags_high :4;
	u64 base_high  :8;
} __attribute__((packed));

struct gdt
{
	u16 limit;
	struct gdt_entry *e;
} __attribute__((packed));

struct tss
{
	u32 resv;
	u64 rsp0, rsp1, rsp2;
	u64 resv2;
	u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
	u64 resv3;
	u32 resv4;
	u32 iobase;
} __attribute__((packed));

static struct gdt gdt;
static struct gdt_entry e[7];
static struct tss tss;
static char kernel_stack[4096] __attribute((aligned (4096)));

static void gdt_load(struct gdt *gdt)
{
	asm volatile("lgdt [rax]" :: "a" (gdt));
	asm volatile("ltr ax" :: "a" (0x2B));
}

static void gdt_set_entry(struct gdt_entry *e, u32 base, u32 limit, u16 flags)
{
	e->base_low   = base & 0xFFFF;
	e->limit_low  = limit & 0xFFFF;
	e->flags_low  = flags & 0xFF;
	e->limit_high = (limit >> 16) & 0xF;
	e->flags_high = (flags >> 12) & 0xF;
	e->base_high  = base >> 24;
}

static void gdt_set_tss_entry(struct gdt_entry *e, struct tss *tss)
{
	u64 tssptr = (u64)tss;

	e[0].base_low = tssptr & 0xFFFF;
	e[0].limit_low = sizeof (*tss) - 1;
	e[0].flags_low = 0x89;
	e[0].limit_high = 0;
	e[0].flags_high = 0x8;
	e[0].base_high = (tssptr >> 24) & 0xFF;

	memset(&e[1], sizeof(*tss), 0);
	*((u32 *)&e[1]) = tssptr >> 32;
}

void gdt_install(void)
{
	gdt.limit = sizeof (e) - 1;
	gdt.e = e;

	gdt_set_entry(&e[0], 0, 0, 0);
	gdt_set_entry(&e[1], 0, 0xFFFFF, 0xAF9B); /* Kernel code */
	gdt_set_entry(&e[2], 0, 0xFFFFF, 0xAF93); /* Kernel data */
	gdt_set_entry(&e[3], 0, 0xFFFFF, 0xAFF3); /* User data */
	gdt_set_entry(&e[4], 0, 0xFFFFF, 0xAFFB); /* User code */
	
	tss.rsp0 = (u64)kernel_stack + sizeof(kernel_stack); 

	gdt_set_tss_entry(&e[5], &tss); /* TSS */
	printf("kernel stack: %lx\n", kernel_stack);
	gdt_load(&gdt);
}

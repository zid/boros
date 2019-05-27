#include <print.h>
#include "cpu.h"
#include "types.h"

#define MAX_INTS 256

static struct idt_entry {
	u16 off_low1;
	u16 selector;
	u16 flags;
	u16 off_low2;
	u32 off_high;
} __attribute__((packed, aligned(16))) idt[MAX_INTS];

static struct idt_info {
	u16 limit;
	struct idt_entry *base;
} __attribute__((packed, aligned(16))) idt_info;

void int_register(u32 num, void *f)
{
	if(num >= MAX_INTS)
		return;

	idt[num].selector = 8;
	idt[num].off_low1 = ((u64)f)&0xFFFF;
	idt[num].off_low2 = (((u64)f)>>16)&0xFFFF;
	idt[num].off_high = (((u64)f)>>32);
	idt[num].flags = 0x8E00;
}

static void __attribute__((interrupt)) int_generic_fault(void *p)
{
	printf("[error]: Fault: %lx\n", p);
	while(1);
}

static void __attribute__((interrupt)) int_pf(void *p)
{
	printf("[error]: pf: %lx\n", p);
	while(1);
}

static void __attribute__((interrupt)) int_gp(void *p)
{
	printf("[error]: General protection fault: %x\n", p);
	asm("cli; 0: hlt; jmp 0b");
}

static void __attribute__((interrupt)) int_noop_master(void *p)
{
	(void)p;
	outb(0x20, 0x20);
}

static void __attribute__((interrupt)) int_noop_slave(void *p)
{
	(void)p;

	outb(0xA0, 0x20);
	outb(0x20, 0x20);
}

void int_install(void)
{
	int i;

	idt_info.limit = (MAX_INTS*16)-1;
	idt_info.base = idt;

	/* Basic #GP handler, easier to debug than a triplefault */
	for(i = 0; i < 20; i++)
		int_register(i, int_generic_fault);
	int_register(0xD, int_gp);
	int_register(0xE, int_pf);

	/* Add a default handler for all IRQs that does nothing */
	for(i = 32; i < 40; i++)
		int_register(i, int_noop_master);
	for(i = 40; i < 48; i++)
		int_register(i, int_noop_slave);

	/* Initialize both PICs */
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	/* IRQs are from interrupt 32 to 48 */
	outb(0x21, 32);
	outb(0xA1, 40);
	/* IRQ 2 is chained */
	outb(0x21, 4);
	outb(0xA1, 2);
	/* PIC is in x86 mode */
	outb(0x21, 1);
	outb(0xA1, 1);
	/* No IRQs are masked */
	outb(0x21, 0);
	outb(0xA1, 0);

	printf("Enabling interrupts.\n");
	/* Load the IDT */
	cpu_lidt((u64)&idt_info);

	/* Enable interrupts and delay*/
	printf("Masking PIT and keyboard\n");

	asm("sti; nop; nop");
	/* Any pending interrupts during boot should have now fired
	 * thanks to the nops and printfs above, so we should be safe
	 * to do our masks without anything getting wedged.
	 */
	outb(0x21, 3);	/* Mask PIT and keyboard for now */
}

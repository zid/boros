#include <types.h>
#include <mem.h>
#include <print.h>
#include <int.h>
#include <pci.h>
#include <acpi.h>
#include <gdt.h>

void kmain(void *mem)
{
	mem_init(mem);

	clear_screen(0);
	printf("Hello from long mode\n");

	gdt_install();
	int_install();
	/* acpi_init(); - PCI-E not delivering interrupts yet */
	pci_init();

	/* No infrastructure for this exists yet, hardcoded here */
	/* Drop into usermode */
	asm(
		"mov eax, 0x23;" /* User data seg + cpl3 */
		"mov ds, ax;"
		"mov es, ax;"
		"mov fs, ax;"
		"mov gs, ax;"
		"push rax;" 
		"push 0;" /* Blank RSP for now, no user stack */ 
		"pushf;"  /* Leak our flags I guess */
		"mov eax, 0x1B;" /* User code seg + cpl3 */
		"push rax;"
		"push 0x200000;"
		"iretq;"
	);
}

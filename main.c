#include <types.h>
#include <mem.h>
#include <print.h>
#include <int.h>
#include <pci.h>
#include <acpi.h>
#include <gdt.h>
#include <syscall.h>
#include <task.h>
#include <wrmsr.h>

void kmain(void *mem)
{
	mem_init(mem);

	clear_screen(0);
	printf("Hello from long mode\n");

	gdt_install();
	int_install();
	/* acpi_init(); - PCI-E not delivering interrupts yet */
	pci_init();
	syscall_install();

	/* No infrastructure for this exists yet, hardcoded here */
	/* Drop into usermode */
	#define IA32_KERNEL_GS_BASE 0xC0000102
	wrmsr(IA32_KERNEL_GS_BASE, (u64)task_get());
	asm(
		"xor ax, ax;"
		"mov fs, ax;"
		"mov gs, ax;"
		"mov eax, 0x18 | 3;" /* User data seg + cpl3 */
		"mov ds, ax;"
		"mov es, ax;"

		"push rax;" 
		"push 0;"
		"push 0x7202;"  /* Rflags - Interrupt + CPL3 */
		"mov eax, 0x20 | 3;" /* User code seg + cpl3 */
		"push rax;"
		"push 0x200000;"
		"iretq;"
	);
}

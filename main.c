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
	acpi_init();
	//pci_init();
	syscall_install();
	task_init();
}

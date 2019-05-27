#include <types.h>
#include <mem.h>
#include <print.h>
#include <int.h>
#include <pci.h>
#include <acpi.h>

void kmain(void *mem)
{
	mem_init(mem);

	clear_screen(0);
	printf("Hello from long mode\n");

	int_install();
	acpi_init();
	//pci_init();

	while(1)
	{
		asm("hlt");
	}
}

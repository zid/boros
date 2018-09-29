#include "types.h"
#include "mem.h"
#include "print.h"
#include "int.h"
#include "pci.h"

void kmain(void *mem)
{
	mem_init(mem);

	clear_screen(0);
	printf("Hello from long mode\n");

	int_install();
	pci_init();

	while(1)
		;
}

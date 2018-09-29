#include "types.h"
#include "mem.h"
#include "print.h"

void kmain(void *mem)
{
	mem_init(mem);

	clear_screen(0);
	printf("Hello from long mode\n");

	while(1)
		;
}

#include "types.h"
#include "mem.h"
#include "print.h"

void kmain(u64 free_page)
{
	init_mem(free_page);

	clear_screen(0);
	printf("Hello from long mode\n");

	while(1)
		;
}

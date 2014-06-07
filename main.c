#include "mem.h"

void kmain(void *free_page, void *e820)
{
	init_mem(free_page, e820);

	while(1)
		;
}

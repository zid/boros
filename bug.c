#include <bug.h>
#include <print.h>

void __attribute__((noreturn)) bug(const char *s)
{
	printf("BUG: %s\n", s);

	while(1)
		asm("cli; 0: hlt; jmp 0b");
}

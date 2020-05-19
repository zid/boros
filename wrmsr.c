#include <wrmsr.h>
#include <types.h>

void wrmsr(u32 msr, u64 value)
{
	asm volatile(
		"mov ecx, %0\n"
		"\tmov edx, %1\n"
		"\tmov eax, %2\n"
		"\twrmsr\n"
		:
		: "g" (msr), "g" ((u32)(value>>32)), "g" ((u32)value)
		: "eax", "edx", "ecx"
	);
}

#include <syscall.h>
#include <types.h>
#include <print.h>
#include <task.h>

#define IA32_STAR  0xC0000081
#define IA32_LSTAR 0xC0000082
#define IA32_CSTAR 0xC0000083
#define IA32_FMASK 0xC0000084

static void __attribute((noinline)) syscall_c(void)
{
	register unsigned int eax asm("eax");

	if(eax == 732)
		printf("Syscall 732\n");
}

static void __attribute__((naked)) syscall_handler(void)
{
	asm volatile ("swapgs");
	asm volatile ("mov rsp, [gs:0]");
	asm volatile ("sti");
	asm volatile ("push r11");
	asm volatile ("push rcx");

	syscall_c();

	asm volatile ("pop rcx");
	asm volatile ("pop r11");
	asm volatile ("cli"); /* swapgs with interrupts enabled sounds bad */
	asm volatile ("swapgs");
	asm volatile ("sysretq");
}

void syscall_install(void)
{
	/* 
	 * Disable Trap and Interrupt flags during syscall entry
	 * otherwise an IRQ might be taken without a valid stack
	 * pointer loaded into rsp.
	 */
	wrmsr(IA32_FMASK, 0x300);
	
	/* Kernel and user CS selectors stored here */
	wrmsr(IA32_STAR,  0x8UL << 32 | 0x10UL << 48);

	/* Entry point */
	wrmsr(IA32_LSTAR, (u64)syscall_handler);
}

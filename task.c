#include <task.h>
#include <mem.h>
#include <wrmsr.h>

#define IA32_KERNEL_GS_BASE 0xC0000102
#define USER_STACK 0x1FF000
#define USER_STACK_SIZE 0x1000
#define KERNEL_STACK_SIZE 0x1000

/* These will need to be dynamic for multiple tasks */
static unsigned char init_task_kernel_stack[KERNEL_STACK_SIZE];
static struct task init_task;

void __attribute((noreturn)) task_init(void)
{
	wrmsr(IA32_KERNEL_GS_BASE, (u64)&init_task);

	mmap(0x1FF000, phys_alloc(PAGE_4K), USER_STACK_SIZE, PT_WR | PT_USER);
	init_task.user_stack = USER_STACK;
	init_task.kernel_stack = (u64)init_task_kernel_stack + KERNEL_STACK_SIZE;

	asm(
		"xor ax, ax;"
		"mov fs, ax;"
		"mov gs, ax;"
		"mov eax, 0x18 | 3;" /* User data seg + CPL3 */
		"mov ds, ax;"
		"mov es, ax;"

		"xor rbx, rbx;"
		"xor rcx, rcx;"
		"xor rdx, rdx;"
		"xor rbp, rbp;"
		"xor r8, r8;"
		"xor r9, r9;"
		"xor r10, r10;"
	        "xor r11, r11;"
		"xor r12, r12;"
		"xor r13, r13;"
		"xor r14, r14;"
		"xor r15, r15;"

		"mov rsp, %0;"

		"push rax;"          /* SS - Same as user data segment */
		"push %0;"           /* RSP - USER_STACK */
		"push 0x7202;"       /* RFLAGS - Interrupt + CPL3 */
		"mov eax, 0x20 | 3;" /* CS - User code segment + CPL3 */
		"push rax;"
		"push 0x200000;"     /* FIXME: _start from ELF */
		"iretq;"
		:
		: "i" (USER_STACK + USER_STACK_SIZE)
	);
	__builtin_unreachable();
}

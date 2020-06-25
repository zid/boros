#include <task.h>

static unsigned char kernel_stack[4096];

static struct task task = 
{
	.stack = (u64)&kernel_stack[4096]
};

struct task *task_get(void)
{
	return &task;
}

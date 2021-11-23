#ifndef TASK_H
#define TASK_H
#include <types.h>

struct task
{
	u64 kernel_stack;
	u64 user_stack;
};

void task_init(void);
#endif

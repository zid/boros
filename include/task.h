#ifndef TASK_H
#define TASK_H
#include <types.h>

struct task
{
	u64 stack;
};

struct task *task_get(void);
#endif

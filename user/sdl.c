#include "cpu.h"

static void debug()
{
}


int sdl_init(void)
{
	return 0;
}

int sdl_update(void)
{
	return 0;
}



unsigned int *sdl_get_framebuffer(void)
{
	return (unsigned int *)0x80000000;
}

void sdl_frame(void)
{
	volatile unsigned int *fb = (void *)0x80000000;
}


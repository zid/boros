void _start(void)
{
	asm("mov rsp, 0x200000; call main");
	while(1);
}


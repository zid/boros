void _start(void)
{
	asm("mov ebx, 3; mov ecx, 4; mov edx, 5; mov edi, 6; mov esi, 7;");
	asm("mov eax, 732; syscall");
	while(1)
		;
}

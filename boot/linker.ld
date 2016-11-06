OUTPUT_FORMAT("elf32-i386")
ENTRY("start")

SECTIONS
{
	. = 0x100000;
	.text :
	{
		boot.o (.text);
		* (.text.startup);
		* (.text);
	}
	_bootloader_size = ALIGN(0x1000) - 0x100000;
	.rodata : ALIGN(8) { * (.rodata*); }
	.data : ALIGN(8) { * (.data); }
	.bss ALIGN(0x1000) :
	{
		_stack_start = . - 0x100000;
		. += 0x1000;
		* (.bss);
	}
	/DISCARD/ : { * (.*); }
}

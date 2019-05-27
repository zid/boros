OUTPUT_FORMAT("elf32-i386")
ENTRY("start")

PHDRS
{
	text PT_LOAD;
	data PT_LOAD;
};

SECTIONS
{
	. = 0x100000;
	.text :
	{
		boot.o (.text);
		* (.text.startup);
		* (.text);
	} :text
	.rodata : ALIGN(8) { * (.rodata*); }
	.data : ALIGN(8) { * (.data); } :data
	.bss ALIGN(0x1000) :
	{
		* (.bss);
		. += 0x1000;
		_stack_start = ALIGN(0x1000);
	}
	
	_bootloader_size = ALIGN(0x1000) - 0x100000;
	/DISCARD/ : { * (.*); }
}

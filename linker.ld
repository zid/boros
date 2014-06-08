OUTPUT_FORMAT("binary")

ENTRY(kmain)

VIRT = 0xFFFFFFFF80000000;

SECTIONS
{
	. = VIRT;
	.text :
	{
		header.o (.text);
		* (.text);
		. = ALIGN(4096);
	}

	.rodata : ALIGN(8) { * (.rodata*); }
	.data : ALIGN(8) { * (.data); }

	.bss ALIGN(0x1000):
	{
		BSS_START = . - VIRT;
		* (.bss);
		. = ALIGN(0x1000);
	}
	BSS_LEN = SIZEOF(.bss);

	/DISCARD/ : {
		* (.*);
	}
}

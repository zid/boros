OUTPUT_FORMAT("binary")

ENTRY(kmain)

VIRT = 0xFFFF800000102000;

SECTIONS
{
	. = VIRT;
	.text :
	{
		header.o (.text);
		* (.text);
		. = ALIGN(4096);
	}

	.rodata : { * (.data); }
	.data : { * (.data); }

	BSS_START = . - VIRT;

	.bss ALIGN(0x1000):
	{
		* (.bss);
	}
	BSS_LEN = SIZEOF(.bss);

	/DISCARD/ : {
		* (.*);
	}
}

OUTPUT_FORMAT("elf64-x86-64")

ENTRY(_start)

VIRT = 0x200000;

PHDRS
{
	text PT_LOAD;
	data PT_LOAD;
}

SECTIONS
{
	. = VIRT;
	.text   : ALIGN(0x1000) { * (.text*);   } :text
	.rodata : ALIGN(0x1000) { * (.rodata*); } :text
	.data   : ALIGN(0x1000) { * (.data);    } :data
	.bss    : ALIGN(0x1000) { * (.bss);     } :data

	/DISCARD/ : { * (.*); }
}

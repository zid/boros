#include <stdlib.h>
#include <device.h>
#include <print.h>
#include <mem.h>
#include <tty.h>

enum {
	VBE_ID,
	VBE_XRES,
	VBE_YRES,
	VBE_BPP,
	VBE_ENABLE,
	VBE_BANK,
	VBE_VIRT_WIDTH,
	VBE_VIRT_HEIGHT,
	VBE_X_OFFSET,
	VBE_Y_OFFSET,
};

static u64 base, mmio;

static void vbe_write(u8 index, u16 value)
{
	*(u16 *)(mmio + 0x500 + index*2) = value;
}

void vbe_setmode(int x, int y)
{
	vbe_write(0, 0xB0C5);
	vbe_write(4, 0);
	vbe_write(1, x);
	vbe_write(2, y);
	vbe_write(3, 32);
	vbe_write(4, 0x41);
}

extern u8 ayame[];
extern u8 unifont_bitmap[];
extern u16 charmap[];

#define PSET(y, x, c) *((u32 *)(base + ((y) * 1024 + (x)) * 4)) = (c);

static void draw_line(int x, int y, const unsigned short *line)
{
	int i, j, glyph_row;
	u8 glyph_byte;
	u16 c;
	int count = 0;
	while(count++ < 80)
	{
		c = *line++;

		if(!c)
			return;

		glyph_row = charmap[c]*16;

		for(j = 0; j < 16; j++)
		{
			glyph_byte = unifont_bitmap[glyph_row + j];

			for(i = 0; i < 8; i++)
			{
				int colour;
				u8 bit;

				bit = !!(glyph_byte & (1 << (8-1-i)));
				if(bit == 1)
					colour = 0xFFFFFF;
				else
				{
					u32 offset = ((y+j)*1024 + (i+x))*4;
					u8 r, g, b;
					r = ayame[offset+2]/2; g = ayame[offset+1]/2; b = ayame[offset+0]/2;
					colour = r<<16 | g << 8 | b;
				}
				PSET(y+j, x+i, colour);
			}
		}

		x += 8;
	}
}

void vbe_init(struct device *d)
{
	u32 mmio_bar, base_bar;

	base_bar = device_read_u32(d, 0x10 + 0*4) & ~0xFUL;
	mmio_bar = device_read_u32(d, 0x10 + 2*4) & ~0xFUL;

	printf("[vbe]: Framebuffer: %lx\n", base_bar);
	printf("[vbe]: MMIO: %lx\n", mmio_bar);

	base = (u64)phys_to_virt(base_bar);
	mmio = (u64)phys_to_virt(mmio_bar);

	mmap(base, base_bar, 16*1024*1024, PT_WR | PT_PRESENT);
	mmap(mmio, mmio_bar, 0x1000, PT_WR | PT_PRESENT);

	/* FIXME: Map framebuffer to usermode */
	mmap(0x80000000, base_bar, 16*1024*1024, PT_WR | PT_USER | PT_PRESENT);

	vbe_setmode(1024, 768);
	memcpy((void *)base, ayame, 1024*768*4);

	int i;

	for(i = 0; i < 40; i++)
		draw_line(0, i*16, tty_get_line(-39 + i));
}

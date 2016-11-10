#include <stdarg.h>
#include "print.h"
#include "stdlib.h"

static unsigned short *vmem = (unsigned short *)0xB8000;

void clear_screen(colour c)
{
	unsigned char colour;
	unsigned int i;

	switch(c)
	{
		case GREEN:
			colour = 0xAA;
		break;
		default:
			colour = 0x00;
		break;
	}

	vmem = (unsigned short *)0xB8000;

	for(i = 0; i < 80*25; i++)
	{
		vmem[i] = colour<<8;
	}
}

static void newline(void)
{
	long p = (long)vmem;

	p = p - ((p - 0xB8000) % 160);
	p += 160;

	vmem = (unsigned short *)p;
}

static void putchar(unsigned char c)
{
	if(c == '\n')
		return newline();

	if(vmem >= (unsigned short *)(0xB8000 + (80 * 25 * 2)))
	{
		vmem -= 80;
		memmove((void *)0xB8000, (void *)0xB8000 + 160, 80 * 24 * 2);
		memset( (void *)0xB8F00, 0, 160);
	}

	*vmem++ = 0xF00 | c;
}

static void puts(const char *s)
{
	while(*s)
		putchar(*s++);
}

static void put_long_hex(unsigned long n)
{
	unsigned long mask = 0xF0000000UL;
	unsigned int shift = 28;
	unsigned int i;
	unsigned char digit;

	for(i = 0; i<8; i++)
	{
		digit = (n&mask)>>shift;
		shift -= 4;
		mask >>= 4;

		putchar(digit["0123456789ABCDEF"]);
	}
}

static void put_long_long_hex(unsigned long long n)
{
	unsigned long long mask = 0xF000000000000000ULL;
	unsigned long shift = 60;
	unsigned long i;
	unsigned char digit;

	for(i = 0; i<16; i++)
	{
		digit = (n&mask)>>shift;
		shift -= 4;
		mask >>= 4;

		putchar(digit["0123456789ABCDEF"]);
	}
}

static void vprint(const char *fmt, va_list ap)
{
	const char *p;
	unsigned char c;
	unsigned int i;
	unsigned long l;
	unsigned long long ll;
	int long_int = 0;

	p = fmt;
	while(1)
	{
		for(; *p && *p != '%'; p++)
			putchar(*p);
		if(!*p)
			return;
		p++;
again:		c = *p++;
		switch(c)
		{
			case '%':
				putchar(c);
			break;
			case 'l':
				long_int = 1;
				goto again;
			break;
			case 'x':
				if(long_int)
				{
					ll = va_arg(ap, long long);
					put_long_long_hex(ll);
				} else {
					l = va_arg(ap, long);
					put_long_hex(l);
				}
				long_int = 0;
			break;
			case 's':
				puts(va_arg(ap, const char *));
			break;
		}
	}
}

void printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vprint(fmt, ap);
	va_end(ap);
}

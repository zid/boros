#include <stdarg.h>
#include "print.h"
#include "stdlib.h"

#define BUF_SIZ 64

unsigned short *vmem = (unsigned short *)0xB8000;

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
	if(c == '\t')
	{
		putchar(' ');
		putchar(' ');
		putchar(' ');
		putchar(' ');
		return;
	}

	if(vmem >= (unsigned short *)(0xB8000 + (80 * 25 * 2)))
	{
		vmem -= 80; /* Short *, not an offset */
		memmove((void *)0xB8000, (void *)0xB8000 + 160, 80 * 24 * 2);
		 memset((void *)0xB8000+80*24*2, 0, 160);
	}

	*vmem++ = 0xF00 | c;
}

static void put_number(unsigned long n, int islong, int base, char pad_char, int width)
{
	unsigned char digit;
	int len;
	char buf[BUF_SIZ];
	char *p = &buf[BUF_SIZ-1];

	if(!islong)
		n = (unsigned int)n;

	do {
		digit = n % base;
		n /= base;
		*p-- = digit["0123456789ABCDEF"];
	} while(n);

	len = &buf[BUF_SIZ-1] - p;
	while(width && len < width)
	{
		*p-- = pad_char;
		len++;
	}

	while(++p != &buf[BUF_SIZ])
		putchar(*p);
}

static void puts(const char *str)
{
	while(*str)
		putchar(*str++);
}

static void vprint(const char *fmt, va_list ap)
{
	const char *p;
	unsigned char c;
	unsigned long l;
	int long_int;
	char pad_char;
	int width;
	int base;
	p = fmt;

	while(1)
	{
		width = 0;
		pad_char = ' ';
		long_int = 0;

		for(; *p && *p != '%'; p++)
			putchar(*p);
		if(!*p)
			return;
		p++;
again:		c = *p++;

		if(c == '0')
		{
			pad_char = '0';
			c = *p++;
		}

		while(c >= '0' && c <= '9')
		{
			width = width*10 + c - '0';
			c = *p++;
		}

		switch(c)
		{
			case '%':
				putchar(c);
			break;
			case 'l':
				long_int = 1;
				goto again;
			break;
			case 'X':
			case 'x':
				base = 16;
				goto print_num;
			case 'd':
				base = 10;
				goto print_num;


			case 's':
				puts(va_arg(ap, const char *));
			break;
			case 'c':
				putchar(va_arg(ap, int));
			break;
			print_num:
				if(long_int)
					l = va_arg(ap, long);
				else
					l = va_arg(ap, int);
				put_number(l, long_int, base, pad_char, width);
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

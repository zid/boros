#include <stdarg.h>

#define BUF_SIZ 256

void serial_puts(const char *s)
{
	int syscall_no = 42;

	asm volatile (
		"syscall"
		: "+a" (syscall_no), "+D" (s)
		: "m" (s[0]), "m" (s[1])
		: "rcx", "rdx", "rsi", "r8", "r9", "r10", "r11"
	);
}

void serial_putchar(char c)
{
	char buf[2];

	buf[0] = c;
	buf[1] = '\0';

	serial_puts(buf);
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
		serial_putchar(*p);
}

static void serial_vprintf(const char *fmt, va_list ap)
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
			serial_putchar(*p);
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
				serial_putchar(c);
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
				serial_puts(va_arg(ap, const char *));
			break;
			case 'c':
				serial_putchar(va_arg(ap, int));
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

void serial_printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	serial_vprintf(fmt, ap);
	va_end(ap);
}

#include <tty.h>
#include <cpu.h>

struct line
{
	unsigned short text[80];
};

static struct line buffer[40];
static int buf_line;
static int buf_pos;

void tty_write_str(const char *str)
{
	char c;

	do
	{
		c = *str++;
		tty_putchar(c);
	} while(c); 
}

void tty_putchar(unsigned char c)
{
	if(c == '\t')
	{
		buffer[buf_line].text[buf_pos+0] = ' ';
		buffer[buf_line].text[buf_pos+1] = ' ';
		buffer[buf_line].text[buf_pos+2] = ' ';
		buffer[buf_line].text[buf_pos+3] = ' ';
		buf_pos += 4;
	}

	if(c == '\n' || buf_pos >= 80)
	{
		buf_pos = 0;
		buf_line++;
		buffer[buf_line].text[0] = 0;
		if(buf_line >= 40)
			buf_line = 0;
	}

	if(c != '\n' && c != '\t')
		buffer[buf_line].text[buf_pos++] = c;
}

unsigned short *tty_get_line(int n)
{
	int line;

	line = buf_line + n;
	if(line > 40)
		line -= 40;
	if(line < 0)
		line += 40;

	return buffer[line].text;
}

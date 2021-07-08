#ifndef _TTY_H
#define _TTY_H
void tty_write_str(const char *);
void tty_putchar(unsigned char c);
unsigned short *tty_get_line(int);
#endif

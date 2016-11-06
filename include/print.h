#ifndef PRINT_H
#define PRINT_H
typedef enum  {
	GREEN = 1
} colour;

void printf(const char *, ...);
void clear_screen(colour c);
#endif

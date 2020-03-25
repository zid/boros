SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
CFLAGS = -fno-builtin -ffreestanding -O2 -no-pie -fno-pic -mno-sse

%.o : %.c
	$(CC) $^ -c -o $@ $(CFLAGS) 

user.bin: $(OBJ)
	ld $(OBJ) -Tlinker.ld -o user.bin

clean:
	-@rm *.o user.bin 2> /dev/null || true

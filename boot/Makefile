SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
ASMSRC = $(wildcard *.asm)
ASMOBJ = $(ASMSRC:.asm=.o)
CFLAGS = -fno-builtin -ffreestanding -O2 -no-pie -fno-pic -mno-sse

%.o : %.c
	$(CC) $^ -c -o $@ $(CFLAGS) -m32

%.o : %.asm
	yasm -felf32 $^ -o $@

boot.bin: $(ASMOBJ) $(OBJ)
	ld $(OBJ) $(ASMOBJ) -melf_i386 -Tlinker.ld -o boot.bin

clean:
	@rm *.o boot.bin

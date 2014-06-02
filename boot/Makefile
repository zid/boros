boot.bin: boot.o
	ld boot.o -melf_i386 -Tlinker.ld -o boot.bin


boot.o: boot.asm
	yasm boot.asm -felf32

clean:
	@rm *.o boot.bin

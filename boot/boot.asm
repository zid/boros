bits 32
global start
extern main

section .text
multiboot:
	.magic dd 0x1BADB002
	.flags dd 3
	.checksum dd 0 - 0x1BADB002 - 3

start:
	cli
	push ebp
	mov ebp, esp

	mov edi, 0xB8000
	xor eax, eax
	mov ecx, 1000
	rep stosd
	push ebx
	push ebx
	jmp main

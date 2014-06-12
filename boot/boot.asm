bits 32
global start

section .text
multiboot:
	.magic dd 0x1BADB002
	.flags dd 3
	.checksum dd 0 - 0x1BADB002 - 3

start:
	cli

	mov eax, [ebx + 4]
	shl eax, 10
	mov esp, eax

	mov edi, 0xB8000
	xor eax, eax
	mov ecx, 1000
	rep stosd

	mov edi, [ebx+24]
	mov esi, [edi]
	mov [kernel_start], esi
	mov edi, [esi+16]
	mov [bss_start], edi
	mov edi, [esi+20]
	mov [bss_length], edi

	mov eax, [kernel_start]
	add eax, [bss_start]
	add eax, [bss_length]
	mov [kernel_end], eax
	mov [free_page], eax

	call clear_bss
	call kernel_map

	xchg bx, bx
	jmp $

clear_bss:
	mov edi, [kernel_start]
	add edi, [bss_start]
	mov ecx, [bss_length]
	shl ecx, 2
	xor eax, eax
	rep stosd
	ret
 
kernel_map:
	mov edi, [kernel_start]
	mov esi, [kernel_end]
	mov edx, 0xFFFFFFFF
	mov eax, 0x80000000
.loop1:
	call map_page
	add edi, 0x1000
	add eax, 0x1000
	cmp edi, edi
	jnz loop1

kernel_start dd 0
kernel_end dd 0
bss_start dd 0
bss_length dd 0
free_page dd 0

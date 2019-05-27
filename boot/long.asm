bits 32
global go_long
extern _stack_start

section .text
go_long:
	mov esi, [esp+8]	;kernel _start lo
	mov edi, [esp+12]	;kernel _start hi
	mov ebx, [esp+16]	;mem info struct

	mov eax, cr4
	or eax, 1 << 5          ;PAE
	and eax, ~(1 << 7)      ;Disable PGE
	mov cr4, eax
	or eax, 1<<7            ;Enable PGE
	mov cr4, eax


	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 11         ;NX
	or eax, 1 << 8          ;Long mode
	wrmsr

	mov eax, [esp+4]	;PML4
	lea ecx, [eax+3]
	mov [eax+4080], ecx
	mov cr3, eax

	mov eax, cr0
	or eax, 1<<31 | 1<<16   ;Paging + Write protect
	mov cr0, eax

	lgdt [gdt]

	mov ax, 16
	mov ds, ax
	mov es, ax
	mov ss, ax

	jmp 8:longmode

bits 64
	longmode:
	mov rax, rdi
	shl rax, 32
	or rax, rsi
	mov esp, _stack_start
	mov edi, ebx	;mem info struct
	jmp rax


align 16
gdt:
	dw 23
	dd gdt + 16
	dd 0
align 16
	dq 0
	dq 0xAF9B000000FFFF
	dq 0xAF93000000FFFF

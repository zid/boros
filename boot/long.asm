bits 32
global go_long
extern _stack_start

section .text
go_long:
	mov esi, [esp+8]
	mov edi, [esp+12]
	mov ebx, [esp+16]

	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	mov eax, [esp+4]
	lea ecx, [eax+3]
	mov [eax+4080], ecx
	mov cr3, eax

	mov eax, cr0
	or eax, 1<<31
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
	add rsp, 0xffffffff80001000
	mov edi, ebx
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

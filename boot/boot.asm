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

	mov [pml4], eax
	add eax, 4096
	mov [free_page], eax

	call clear_bss
	call kernel_map
	call bootstrap_map
	call stack_map
	call page_table_map

	;Enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	;Enable long mode bit
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	;Load the page tables
	mov eax, [pml4]
	mov cr3, eax

	;Enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	;Load the segment tables
	lgdt [gdt]

	;Load the segment selectors
	mov ax, 16
	mov ds, ax
	mov es, ax
	mov ss, ax

	;Jump to long mode
	jmp 8:longmode

bits 64
	longmode:
	mov rax, 0xFFFFFFFF80000000
	mov rsp, rax
	xchg bx, bx
	jmp rax

	hlt
	jmp $

bits 32
clear_bss:
	mov edi, [kernel_start]
	add edi, [bss_start]
	mov ecx, [bss_length]
	shr ecx, 2
	xor eax, eax
	rep stosd
	ret

;Identity map the page tables we made
page_table_map:
	mov eax, [pml4]
	mov edi, eax
	mov esi, [free_page]
	xor edx, edx
.loop1:
	call map_page
	add edi, 0x1000
	add eax, 0x1000
	cmp edi, esi
	jnz .loop1
ret

;Map a page for the kernel stack
stack_map:
	mov edi, [free_page]
	add dword [free_page], 4096
	mov edx, 0xFFFFFFFF
	mov eax, 0x7FFFF000
	call map_page
ret

;Identity map the bootstrap so we can jmp 8:longmode
bootstrap_map:
	mov edi, multiboot
	mov esi, [kernel_start]
	xor edx, edx
	mov eax, 0x100000
.loop1:
	call map_page
	add edi, 0x1000
	add eax, 0x1000
	cmp edi, esi
	jnz .loop1
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
	cmp edi, esi
	jnz .loop1
ret


;Map edi to edx:eax
map_page:
	push edi

	mov ebx, [pml4]

	mov ecx, edx
	shr ecx, 7
	and ecx, 0x1FF
	shl ecx, 3		;PML4E

	mov edi, [ebx+ecx]	;PDPT
	test edi, edi
	jnz .skipnewpdpt
	call newpage
	or edi, 3		;Present + writeable
	mov [ebx+ecx], edi
.skipnewpdpt:
	and edi, 0xFFFFF000	;Clear flags from PML4E

	mov ebx, edi		;PDPT

	;Calculate PDPT offset into ecx
	mov ecx, eax
	shrd ecx, edx, 30
	and ecx, 0x1FF
	shl ecx, 3
	mov edi, [ebx+ecx]	;PDPTE
	test edi, edi
	jnz .skipnewpd
	call newpage
	or edi, 3
	mov [ebx+ecx], edi
.skipnewpd:
	and edi, 0xFFFFF000	;Clear flags from PDPTE
	mov ebx, edi		;PD

	mov ecx, eax
	shr ecx, 21
	and ecx, 0x1FF
	shl ecx, 3

	mov edi, [ebx+ecx]	;PDE
	test edi, edi
	jnz .skipnewpt
	call newpage
	or edi, 3
	mov [ebx+ecx], edi
.skipnewpt:
	and edi, 0xFFFFF000	;Clear flags from PDE
	mov ebx, edi		;PT
	mov ecx, eax
	shr ecx, 12
	and ecx, 0x1FF
	shl ecx, 3

	pop edi
	push edi
	or edi, 3		;Present + writeable
	mov [ebx+ecx], edi
	pop edi
ret

;Allocates and clears a new page
newpage:
	push esi
	push ecx
	push eax

	mov esi, [free_page]
	add dword [free_page], 4096

	mov edi, esi
	mov ecx, 1000
	xor eax, eax
	rep stosd
	mov edi, esi

	pop eax
	pop ecx
	pop esi
ret


kernel_start dd 0
kernel_end dd 0
bss_start dd 0
bss_length dd 0
free_page dd 0
pml4 dd 0

align 16
gdt:
	dw 23
	dd gdt + 16
	dd 0

align 16
gdt_table:
	dq 0
	dq 0xAF9B000000FFFF
	dq 0xAF93000000FFFF

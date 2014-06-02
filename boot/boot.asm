bits 32
global start

section .text

multiboot:
	.magic dd 0x1BADB002
	.flags dd 3
	.checksum dd 0 - 0x1BADB002 - 3


start:
	cli

	mov eax, [ebx+4]	;mem_lower
	shl eax, 10		;Convert kB to an address
	mov esp, eax		;Put stack at top of lowmem

	;Multiboot information
	push ebx

	mov edi, [ebx+24]	;mods_addr
	mov eax, [edi]		;first module start

	push dword [eax+16]	;bss start
	push dword [eax+20]	;bss length
	push dword [edi+4]	;module end address
	push eax		;module start address

	;Clear the text console
	mov edi, 0xB8000
	xor eax, eax
	mov ecx, 80*25*2
	rep stosd

	;Enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	;EFER.LM
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	;Map the kernel to -2GB in 64bit space
	call kernel_map

	;Identity map the bootstrap so we can carry on
	;executing after the switch to long mode.
	; rdi = next free page
	call bootstrap_map
	mov eax, edi
	mov cr3, eax

	;Enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	;Load 64bit GDT
	lgdt [gdt]

	;Reload segments
	jmp 8:longmode

bits 64
longmode:
	mov ax, 16
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	xchg bx, bx
	mov rax, 0xFFFFFFFF80000000
	jmp rax

bits 32
clear_page:
	mov edi, [esp+4]
	push eax
	push ecx
	shl ecx, 12
	xor eax, eax
	rep stosd
	pop ecx
	pop eax
retn 4

kernel_map:
	;Calculate the size of the kernel image
	mov eax, [esp+4]	;kernel start
	mov ebx, [esp+8]	;kernel end
	add ebx, [esp+12]	;end + bss length

	;Make a set of page tables to map -2GB to the kernel image
	; (512GB)  ->   (1GB)   -> (2MB) -> (4kB)
	;PML4[511] -> PDPT[510] -> PD[0-n] -> PT * n

	;How many pages are we going to need for page tables?
	mov ecx, ebx
	sub ecx, eax
	shr ecx, 21
	inc ecx			;Number of 2MB regions the kernel spans
	add ecx, 3		;PML4 + PDPT + PD

	push ecx		;Save no. of pages

	;Zero out the page table pages
	; ebx = end of kernel
	; ecx = number of pages
	push ebx
	call clear_page

	;Point PMl4[511] (-512GB) to the PDPT
	; PML4 = end of kernel
	; PDPT = end of kernel + 4096
	mov edi, ebx
	add edi, 4096
	mov esi, edi
	or esi, 3
	mov [ebx+4088], esi

	;Point PDPT[510] (-2GB) to the PD (2MB)
	; edi = PDPT
	mov esi, edi
	add esi, 4096
	mov edx, esi
	or edx, 3
	mov [edi+4080], edx 

	;Write the addresses of the PTs into the PD
	; esi = PD
	; ecx = no. of PTs
	mov edi, esi		
	mov edx, esi
	push eax
.loop:
	add edi, 4096		;Address of page table
	mov eax, edi
	or eax, 3		;present + writeable flags
	mov dword [edx], eax
	add edx, 8
	dec ecx
	jnz .loop
	pop eax

	;Number of 4kB pages the kernel occupies
	pop ecx			;Restore no. of pages of page tables
	mov edx, ebx
	sub edx, eax
	shr edx, 12		;Calculate size of kernel in pages
	add ecx, edx		;Add number of page tables pages

	;Point all the PT entries to the physical pages the kernel occupies
	; ecx = Page count
	; ebx = end of kernel
	; eax = start of kernel
	mov esi, ebx		;End of kernel
	add esi, 0x3000		;PT = end of kernel + PML + PDPT + PD
	push esi
	push ecx
	mov edi, eax		;Start of kernel
.loop2:
	mov eax, edi
	or eax, 3
	mov [esi], eax
	add edi, 0x1000
	add esi, 8
	dec edx
	jnz .loop2

	pop ecx
	pop esi
	mov edi, ebx
	shl ecx, 12
	add esi, ecx
	;esi = End of page tables
retn 16

bootstrap_map:
	; esi = free page
	; edi = PML4
	; 'multiboot' symbol = start of bootstrap
	push edi

	;Map 'ebx' count pages starting from 'eax' into page tables at 'esi'
	; and insert into the existing PML4 at 'edi'

	;Asume the bootstrap is below 1GB, so we can ignore the PML4 and PDPT.
	;PML4[0] = new PDPT
	mov eax, esi
	or eax, 3		;Present, Writeable
	mov [edi], eax		;PML4[0]
	mov edi, esi
	add esi, 4096

	;new PDPT[0] = new PD
	mov eax, esi
	or eax, 3		;Present, Writeable
	mov [edi], eax		;PDPT[0]

	;The PD selects which 2MB chunk within the first 1GB we need.
	mov ecx, multiboot
	shr ecx, 21		;Divide by 2MB
	shl ecx, 3		;Turn into a 0-511 index

	;ecx = which 2MB 
	mov eax, esi
	add eax, ecx
	add esi, 4096		;New PT
	mov edi, esi
	or edi, 3
	mov [eax], edi		;New PD[n] = new PT


	;Write mappings into the PT
	mov eax, multiboot
	mov ecx, length
	shr ecx, 12
	inc ecx			;Number of pages the bootstrap occupies
	mov ebx, eax
	shr ebx, 12
	shl ebx, 3
	add esi, ebx
.loop:
	mov edx, eax
	or edx, 3
	mov [esi], edx		;Page of bootstrap + writeable + present

	add esi, 8
	add eax, 4096
	dec ecx
	jnz .loop

	pop edi			;Restore PML4 address
ret

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
length equ $ - multiboot

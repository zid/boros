global get_cr3, set_cr3, outb
global out32, in32, cpu_lidt
global inb

get_cr3:
	mov rax, cr3
	ret

set_cr3:
	mov rax, rdi
	mov cr3, rax
	ret

outb:
	mov rdx, rdi
	mov rax, rsi
	out dx, al
	ret
inb:
	mov rdx, rdi
	in al, dx
	ret
out32:
	mov rdx, rdi
	mov rax, rsi
	out dx, eax
	ret

in32:
	mov rdx, rdi
	in eax, dx
	ret

cpu_lidt:
	lidt [rdi]
	ret

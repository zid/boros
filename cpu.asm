global get_cr3, set_cr3, outb

get_cr3:
	mov rax, cr3
	ret

cpu_load_cr3:
	mov rax, rdi
	mov cr3, rax
	ret
outb:
	mov rdx, rdi
	mov rax, rsi
	out dx, al
	ret

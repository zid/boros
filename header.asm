bits 32
extern BSS_START, BSS_LEN, kmain
section .text
jmp kmain

align 16
dd BSS_START
dd BSS_LEN
dq kmain

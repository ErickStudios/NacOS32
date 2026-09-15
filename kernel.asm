bits 32

section .multiboot
align 8

multiboot_header:
    dd 0xE85250D6
    dd 0
    dd multiboot_header_end - multiboot_header
    dd -(0xE85250D6 + 0 + (multiboot_header_end - multiboot_header))

    dw 0
    dw 0
    dd 8

multiboot_header_end:

section .text
global start
extern k_main

start:
    cli
	call k_main
	hlt

.hang:
    hlt
    jmp .hang
; Declare constants for the multiboot header.
MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
USE_GFX  equ  0
MBFLAGS  equ  MBALIGN | MEMINFO | USE_GFX ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + MBFLAGS) ; checksum of above, to prove we are multiboot
                                ; CHECKSUM + MAGIC + MBFLAGS should be Zero (0)

section .multiboot
align 4
	dd MAGIC
	dd MBFLAGS
	dd CHECKSUM
	dd 0, 0, 0, 0, 0
	
	dd 0
	dd 800
	dd 600
	dd 32

section .bss
align 16
stack_bottom:
	resb 16384 * 8 ; 16 * 8 KiB  is reserved for stack
stack_top:

section .boot

global _start
_start: 
	mov eax, (initial_page_dir - 0xC0000000)
	mov cr3, eax

	mov ecx, cr4
	or ecx, 0x10
	mov cr4, ecx

	mov ecx, cr0
	or ecx, 0x80000000
	mov cr0, ecx

	jmp higher_half

section .text
higher_half:
	mov esp, stack_top
	push ebx
	push eax
	XOR ebp, ebp
	extern kernel_main
	call kernel_main
halt:
	halt
	jmp halt

section .data
align 4096
global initial_page_dir
initial_page_dir:
	dd 10000011b
	times 768-1 dd 0

	dd (0 << 22) | 10000011b
	dd (1 << 22) | 10000011b
	dd (2 << 22) | 10000011b
	dd (3 << 22) | 10000011b
	times 256-4 dd 0
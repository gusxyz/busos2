section .init
global _init
_init:
    push ebp
    mov ebp, esp
    ; The linker and crtbegin.o will insert constructor calls here

section .fini
global _fini
_fini:
    push ebp
    mov ebp, esp
    ; The linker and crtbegin.o will insert destructor calls here

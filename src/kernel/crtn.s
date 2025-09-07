section .init
    ; The linker and crtend.o have placed their code before this point
    pop ebp
    ret

section .fini
    ; The linker and crtend.o have placed their code before this point
    pop ebp
    ret
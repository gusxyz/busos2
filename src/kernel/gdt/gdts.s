global gdt_flush

gdt_flush:
    MOV eax, [esp+4]
    LGDT [eax]

    MOV EAX, 0x10
    MOV ds, ax
    MOV es, ax
    MOV fs, ax
    MOV gs, ax
    MOV ss, ax
    JMP 0x08:.flush
.flush:
    RET

global tss_flush
tss_flush:
    MOV ax, 0x2B ; offset assosciated with task segment
    LTR ax ;load task state register
    RET
    
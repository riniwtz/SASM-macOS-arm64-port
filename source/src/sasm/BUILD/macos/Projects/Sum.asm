%include "io64.inc"

section .text
global CMAIN
CMAIN:
    ; Enter two numbers in SASM's Input pane, for example: 20 22
    GET_DEC 8, rax
    GET_DEC 8, rcx
    add rax, rcx
    PRINT_DEC 8, rax
    NEWLINE
    xor eax, eax
    ret

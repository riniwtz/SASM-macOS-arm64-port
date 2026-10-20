%include "io64.inc"

section .text
global CMAIN
CMAIN:
    PRINT_STRING "Hello from SASM on macOS!"
    NEWLINE
    xor eax, eax
    ret

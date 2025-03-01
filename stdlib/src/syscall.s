    .text
    .global _syscall

_syscall:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a3, a4
    mv a4, a5
    mv a5, a6
    ecall
    ret

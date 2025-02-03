    .text 
    .globl _start
    .type _start, @function

_start:
    
    call main
    
    # exit
    li a7, 93
    ecall

    .size _start, .-_start 



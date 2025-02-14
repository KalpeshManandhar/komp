    .text 
    .globl _start
    .type _start, @function

_start:
    
    # argc (first value on the stack)
    ld a0, 0(sp)  # a0 = argc

    # argv (second value on the stack)
    add a1, sp, 8  # a1 = argv (pointer to array of strings)

    # find envp (after argv)
    mv a2, a1
find_envp:
    ld t0, 0(a2)
    add a2, a2, 8
    bnez t0, find_envp  # until argv[i] == NULL

    # call main(argc, argv, envp)
    call main

    # exit
    li a7, 93
    ecall

    .size _start, .-_start 



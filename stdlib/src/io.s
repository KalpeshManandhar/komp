    .text
    .globl write

# long write(unsigned int fd, const char *buf, unsigned long long count);
write:
    li a7, 64
    ecall
    ret

    

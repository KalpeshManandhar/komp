    .text
    .globl write, read

# long write(unsigned int fd, const char *buf, unsigned long long count);
write:
    li a7, 64
    ecall
    ret

# long read(unsigned int fd, char *buf, unsigned long long count);
read:
    li a7, 63
    ecall
    ret


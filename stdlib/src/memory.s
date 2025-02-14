    .text
    .globl brk, mmap, munmap, mprotect

# void *brk(void *addr); (used for heap memory management)
brk:
    li a7, 214
    ecall
    ret

# void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
mmap:
    li a7, 222
    ecall
    ret

# int munmap(void *addr, size_t length);
munmap:
    li a7, 215
    ecall
    ret

# int mprotect(void *addr, size_t len, int prot);
mprotect:
    li a7, 226
    ecall
    ret

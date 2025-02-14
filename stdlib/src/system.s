    .text
    .globl uname, chdir, getcwd

# int uname(struct utsname *buf);
uname:
    li a7, 160
    ecall
    ret

# int chdir(const char *path);
chdir:
    li a7, 49
    ecall
    ret

# char *getcwd(char *buf, size_t size);
getcwd:
    li a7, 17
    ecall
    ret

    .text
    .globl open, close, fstat, lseek, unlink, rename, mkdir, rmdir

# int open(const char *pathname, int flags, mode_t mode);
open:
    li a7, 1024
    ecall
    ret

# int close(int fd);
close:
    li a7, 57
    ecall
    ret

# int fstat(int fd, struct stat *statbuf);
fstat:
    li a7, 80
    ecall
    ret

# off_t lseek(int fd, off_t offset, int whence);
lseek:
    li a7, 62
    ecall
    ret

# int unlink(const char *pathname);
unlink:
    li a7, 1026
    ecall
    ret

# int rename(const char *oldpath, const char *newpath);
rename:
    li a7, 1030
    ecall
    ret

# int mkdir(const char *pathname, mode_t mode);
mkdir:
    li a7, 1030
    ecall
    ret

# int rmdir(const char *pathname);
rmdir:
    li a7, 1031
    ecall
    ret

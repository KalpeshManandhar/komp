    .text
    .globl gettimeofday, nanosleep

# int gettimeofday(struct timeval *tv, struct timezone *tz);
gettimeofday:
    li a7, 169
    ecall
    ret

# int nanosleep(const struct timespec *req, struct timespec *rem);
nanosleep:
    li a7, 101
    ecall
    ret

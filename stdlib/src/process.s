    .text
    .globl exit, getpid, kill, fork, execve, wait4, waitpid

# void exit(int status);
exit:
    li a7, 93
    ecall
    ret

# pid_t getpid(void);
getpid:
    li a7, 172
    ecall
    ret

# int kill(pid_t pid, int sig);
kill:
    li a7, 129
    ecall
    ret

# pid_t fork(void);
fork:
    li a7, 220
    li a0, 17 
    mv a1, zero
    mv a2, zero
    mv a3, zero
    mv a4, zero
    ecall
    ret

# int execve(const char *filename, char *const argv[], char *const envp[]);
execve:
    li a7, 221
    ecall
    ret

# pid_t wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage);
waitpid:
    li a7, 260
    mv a3, zero
    ecall
    ret
# pid_t wait4(pid_t pid, int *wstatus, int options, struct rusage *rusage);
wait4:
    li a7, 260
    ecall
    ret
    

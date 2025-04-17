#include <syscall.h>
#define pid_t long long


void exit(int status);
pid_t getpid();
int kill(pid_t pid, int sig);
pid_t fork();
int execve(const char *filename, char *const *argv, char *const *envp);
pid_t waitpid(pid_t pid, int *wstatus, int options);
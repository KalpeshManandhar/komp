void *brk(void *addr);
void *mmap(void *addr, size_t length, int prot, int flags, int fd, long long int offset);
int munmap(void *addr, size_t length);
int mprotect(void *addr, size_t len, int prot);
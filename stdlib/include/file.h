#define mode_t int
#define off_t long long

int open(const char *pathname, int flags, mode_t mode);
int close(int fd);
int fstat(int fd, struct stat *statbuf);
off_t lseek(int fd, off_t offset, int whence);
int unlink(const char *pathname);
int rename(const char *oldpath, const char *newpath);
int mkdir(const char *pathname, mode_t mode);
int rmdir(const char *pathname);
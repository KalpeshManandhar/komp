typedef long unsigned int size_t;

extern int putchar(int c);
extern int puts(const char* str);
extern int printf(const char* format, ...);
extern int getchar(void);
extern char* fgets(char* str, int n, int stream);
extern int sscanf(const char* str, const char* format, ...);
extern long write(unsigned int fd, const char *buf, unsigned long long count);
extern long read(unsigned int fd, char *buf, unsigned long long count);

typedef struct FILE FILE;
extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int fgetc(FILE* stream);
int fputc(int c, FILE* stream);
int fprintf(FILE* stream, const char* format, ...);
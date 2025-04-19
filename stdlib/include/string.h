typedef unsigned long size_t;

size_t strlen(const char* str){
    size_t a = 0;
    while (*str++ != 0){
        a++;
    }
    return a;
}

int strcmp(const char* s1, const char* s2){
    while (*s1 != 0 && *s2 != 0){
        if (*s1 != *s2){
            if (*s1 < *s2){
                return -1;
            }
            return 1;
        }
        s1++;
        s2++;
    }
    return 0;
}
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);
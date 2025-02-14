#include <io.h>
/*
    Test entry point, and input arguments.
*/

int strlen(const char* str){
    char *start = str;
    while (*str != 0){
        str = str + 1;
    }
    return str - start;
}


int main(int argc, char **argv){
    int i = 0;

    for (i=0; i<argc; i = i + 1){
        write(1, argv[i], strlen(argv[i]));
        write(1, "\n", 1);
    }

    return argc;
}
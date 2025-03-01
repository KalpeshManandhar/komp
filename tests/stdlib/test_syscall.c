#include <syscall.h>

int main(){
    char *argv[2] = {"/bin/ls", 0};
    char *envp[1] = {0};
    int ret = _syscall(221, "/bin/ls", argv, envp, 0, 0, 0);
    
    if (ret < 0){
        _syscall(64, 1, "something went wrong\n", 21, 0, 0, 0);
    }

    return ret;
}

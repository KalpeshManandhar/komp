#include <process.h>
#include <io.h>

int main(){
    char *argv[2] = {"/bin/ls", 0};
    char *envp[1] = {0};
    int ret = execve("/bin/ls", argv, envp);
    
    if (ret < 0){
        write(1, "something went wrong\n", 21);
    }

    return ret;
}

#include <process.h>
#include <io.h>
#include <system.h>
#include <string.h>

#define COMMAND_BUFFER_SIZE 150
#define CWD_BUFFER_SIZE 150

int main(){

    char workingDir[CWD_BUFFER_SIZE];
    getcwd(workingDir, CWD_BUFFER_SIZE);
    
    char commandBuffer[COMMAND_BUFFER_SIZE];

    while (1){
        write(1, workingDir, strlen(workingDir));
        write(1, " :: > ", 6);

        int len = read(0, commandBuffer, COMMAND_BUFFER_SIZE-1);
        commandBuffer[len - 1] = 0;
        



        if (strcmp(commandBuffer, "exit") == 0){
            break;
        }



        int pid = fork();

        // child process
        if (pid == 0){
            char * args[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
            char * env[1] = {0};
            
            // tokenize with space delimiter
            int i = 0, j = 0;
            while (i < len){
                // skip white spaces
                while (i < len && commandBuffer[i] == ' '){
                    commandBuffer[i++] = 0;
                }
                
                if (i < len){
                    args[j++] = &commandBuffer[i];
                }

                // skip non white spaces
                while (i < len && commandBuffer[i] != ' '){
                    i++;
                }
            
            }

            for (i = 0; i<j; i++){
                write(1, args[i], strlen(args[i]));
                write(1, "\n", 1);
            }



            int a = execve(args[0], args, env);
            write(1, "\nError\n", strlen("Error\n"));
            return a;
        }
        else if (pid < 0){
            write(1, "\nfork failed\n", strlen("fork failed\n"));
        }
        else {
            int wstatus = 0;

            while (1){
                int ret = waitpid(pid, &wstatus, 2);

                if (ret == -1){
                    write(1, "\nWaitpid error\n", strlen("Waitpid error\n"));
                    return 1;
                }
                if ((((wstatus) & 0x7f) == 0)){
                    write(1, "\nChild process exited.\n", strlen("Child process exited.\n"));
                    break;
                }
            }

        }
    }


    return 0;
}
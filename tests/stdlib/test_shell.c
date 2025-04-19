
#include <process.h>
#include <io.h>
#include <system.h>
#include <string.h>

#define COMMAND_BUFFER_SIZE 150
#define CWD_BUFFER_SIZE 150

int main(){

    char workingDir[CWD_BUFFER_SIZE];
    
    char commandBuffer[COMMAND_BUFFER_SIZE];
    
    while (1){
        getcwd(workingDir, CWD_BUFFER_SIZE);
        write(1, workingDir, strlen(workingDir));
        write(1, " :: > ", 6);

        int len = read(0, commandBuffer, COMMAND_BUFFER_SIZE-1);
        commandBuffer[len - 1] = 0;
        

        if (strcmp(commandBuffer, "exit") == 0){
            break;
        }

        char * args[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
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

        
        // Handle 'cd' as a built-in command
        if (!args[0]){
            continue;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == (char*)0) {
                write(1, "cd: missing operand\n", 20);
            } else if (chdir(args[1]) != 0) {
                write(1, "cd: no such directory\n", 23);
            }
            continue;
        }

        int pid = fork();

        // child process
        if (pid == 0){
            char * env[1] = {0};
            
            

            // for (i = 0; i<j; i++){
            //     write(1, args[i], strlen(args[i]));
            //     write(1, "\n", 1);
            // }


            int a = execve(args[0], args, env);
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
                    write(1, "\nWaitpid error\n", strlen("\nWaitpid error\n"));
                    return 1;
                }
                if ((((wstatus) & 0x7f) == 0)){
                    write(1, "\nChild process exited.\n", strlen("\nChild process exited.\n"));
                    break;
                }
            }
        }
    }

    return 0;
}
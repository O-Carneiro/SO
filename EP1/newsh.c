#include "newsh.h"

char *promptUser(char *username){

    char *command = (char *)malloc(sizeof(char) * 1024);
  
    time_t now;
    struct tm *ts;
    char timeBuffer[15];
    char *readlinebuffer = (char *)malloc(1024);


    now = time(NULL);

    ts = localtime(&now);
    strftime(timeBuffer, sizeof(timeBuffer), " [%H:%M:%S]: ", ts);

    strcpy(readlinebuffer,username);
    strcat(readlinebuffer, timeBuffer);

    command = readline(readlinebuffer);

    free(readlinebuffer);

    return command;

}

int handleCommand(char *command){

    char *token = strtok(command, " ");
    int status;

    if(token[0] == '.' || token[0] == '/'){
        if(fork() != 0){
            waitpid(-1, &status, 0);
        }
        else{
            int i = 1;
            char*args[] = {token, NULL, NULL, NULL, NULL};
            char*aux = strtok(NULL, " \n") ;
            while(aux != NULL){
                args[i] = aux;
                i++;
                aux = strtok(NULL, " \n"); 
            }
            execve(args[0], args, NULL);
        }
    }
    else{
        if(token[0] == 'c'){
            chdir(strtok(NULL, " \n"));
        }
        else if(token[0] == 'r'){
            unlink(strtok(NULL, " \n"));
        }
        else{
            struct utsname uts;
            uname(&uts);
            printf("%s %s %s %s %s\n", uts.sysname,uts.nodename, uts.release, uts.version, uts.machine);

        }
    }

    free(command);

    return 0;
}

int main(){
    
    struct passwd* passwd;
    char *username;

    passwd = getpwuid(getuid());
    
    username = passwd->pw_name;
    
    while(TRUE){
        char *command = promptUser(username);
        if(strcmp(command, "exit") == 0)
            break;
        add_history(command);
        handleCommand(command);
    }

    return 0;
}

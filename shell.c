#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024

// Komutu çalıştıran fonksiyon
void execute_command(char *command) {
    char *args[64];
    int i = 0;
    
    args[i] = strtok(command, " ");
    while(args[i] != NULL && i < 63) {
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;
    
    pid_t pid = fork();
    
    if(pid < 0) {
        perror("Fork hatası");
        return;
    }
    else if(pid == 0) {
        // Child process
        if(execvp(args[0], args) < 0) {
            printf("Komut bulunamadı: %s\n", args[0]);
            exit(1);
        }
    }
    else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

int main() {
    char command[MAX_CMD_LEN];
    
    printf("Mini Shell başlatıldı. Çıkmak için 'exit' yazın.\n");
    
    while(1) {
        printf("myshell> ");
        fflush(stdout);
        
        if(fgets(command, MAX_CMD_LEN, stdin) == NULL) {
            break;
        }
        
        command[strcspn(command, "\n")] = 0;
        
        if(strcmp(command, "exit") == 0) {
            printf("Çıkılıyor...\n");
            break;
        }
        
        if(strlen(command) == 0) {
            continue;
        }
        
        execute_command(command);
    }
    
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024

// Tek bir komutu çalıştır ve exit code döndür
int execute_command(char *command) {
    char *args[64];
    int i = 0;
    
    // Komutu parse et
    args[i] = strtok(command, " ");
    while(args[i] != NULL && i < 63) {
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;
    
    // Boş komut kontrolü
    if(args[0] == NULL) {
        return 0;
    }
    
    // Built-in: cd komutu
    if(strcmp(args[0], "cd") == 0) {
        if(args[1] == NULL) {
            chdir(getenv("HOME"));
        }
        else {
            if(chdir(args[1]) != 0) {
                perror("cd hatası");
                return 1;  // Hata
            }
        }
        return 0;  // Başarılı
    }
    
    // Built-in: exit komutu
    if(strcmp(args[0], "exit") == 0) {
        printf("Çıkılıyor...\n");
        exit(0);
    }
    
    // Normal komutlar için fork-exec
    pid_t pid = fork();
    
    if(pid < 0) {
        perror("Fork hatası");
        return 1;
    }
    else if(pid == 0) {
        // Child process
        if(execvp(args[0], args) < 0) {
            printf("Komut bulunamadı: %s\n", args[0]);
            exit(127);
        }
    }
    else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        
        // Child'ın exit code'unu döndür
        if(WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 1;
    }
    
    return 0;
}

// Operatörleri (&&, ||) handle et
void handle_operators(char *command) {
    char *commands[100];
    char *operators[100];
    int cmd_count = 0;
    
    // Komutu && ve || ile parçala
    char *token = command;
    char *start = command;
    
    while(*token) {
        // && kontrolü
        if(token[0] == '&' && token[1] == '&') {
            *token = '\0';  // Komutu kes
            commands[cmd_count] = start;
            operators[cmd_count] = "&&";
            cmd_count++;
            token += 2;
            while(*token == ' ') token++;  // Boşlukları atla
            start = token;
            continue;
        }
        
        // || kontrolü
        if(token[0] == '|' && token[1] == '|') {
            *token = '\0';
            commands[cmd_count] = start;
            operators[cmd_count] = "||";
            cmd_count++;
            token += 2;
            while(*token == ' ') token++;
            start = token;
            continue;
        }
        
        token++;
    }
    
    // Son komutu ekle
    commands[cmd_count] = start;
    cmd_count++;
    
    // Komutları sırayla çalıştır
    int exit_code = 0;
    for(int i = 0; i < cmd_count; i++) {
        // Boşlukları temizle
        char *cmd = commands[i];
        while(*cmd == ' ') cmd++;
        
        if(strlen(cmd) == 0) continue;
        
        // İlk komut veya operatöre göre çalıştır
        if(i == 0) {
            exit_code = execute_command(cmd);
        }
        else {
            char *op = operators[i-1];
            
            if(strcmp(op, "&&") == 0) {
                // Önceki başarılıysa çalıştır
                if(exit_code == 0) {
                    exit_code = execute_command(cmd);
                }
            }
            else if(strcmp(op, "||") == 0) {
                // Önceki başarısızsa çalıştır
                if(exit_code != 0) {
                    exit_code = execute_command(cmd);
                }
            }
        }
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
        
        if(strlen(command) == 0) {
            continue;
        }
        
        handle_operators(command);
    }
    
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CMD_LEN 1024

// SIGCHLD sinyal handler - zombie process'leri temizler
void sigchld_handler(int signo) {
    // Tüm biten child process'leri temizle
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// Tek bir komutu çalıştır ve exit code döndür
int execute_command(char *command, int background) {
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
                return 1;
            }
        }
        return 0;
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
        if(background) {
            // Arka planda çalışıyor - beklemeden devam et
            printf("[%d] %s\n", pid, args[0]);
            return 0;
        }
        else {
            // Ön planda - bitmesini bekle
            int status;
            waitpid(pid, &status, 0);
            
            if(WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return 1;
        }
    }
    
    return 0;
}

// Operatörleri (&&, ||, &) handle et
void handle_operators(char *command) {
    // & kontrolü (arka plan)
    int background = 0;
    int len = strlen(command);
    
    // Komutun sonunda & var mı?
    if(len > 0 && command[len-1] == '&') {
        background = 1;
        command[len-1] = '\0';  // &'i sil
        
        // Sondaki boşlukları temizle
        len--;
        while(len > 0 && command[len-1] == ' ') {
            command[len-1] = '\0';
            len--;
        }
    }
    
    char *commands[100];
    char *operators[100];
    int cmd_count = 0;
    
    // Komutu && ve || ile parçala
    char *token = command;
    char *start = command;
    
    while(*token) {
        // && kontrolü
        if(token[0] == '&' && token[1] == '&') {
            *token = '\0';
            commands[cmd_count] = start;
            operators[cmd_count] = "&&";
            cmd_count++;
            token += 2;
            while(*token == ' ') token++;
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
        char *cmd = commands[i];
        while(*cmd == ' ') cmd++;
        
        if(strlen(cmd) == 0) continue;
        
        // Background sadece son komutta geçerli
        int is_last = (i == cmd_count - 1);
        int bg = (is_last && background) ? 1 : 0;
        
        if(i == 0) {
            exit_code = execute_command(cmd, bg);
        }
        else {
            char *op = operators[i-1];
            
            if(strcmp(op, "&&") == 0) {
                if(exit_code == 0) {
                    exit_code = execute_command(cmd, bg);
                }
            }
            else if(strcmp(op, "||") == 0) {
                if(exit_code != 0) {
                    exit_code = execute_command(cmd, bg);
                }
            }
        }
    }
}

int main() {
    char command[MAX_CMD_LEN];
    
    // SIGCHLD sinyal handler'ı kur
    signal(SIGCHLD, sigchld_handler);
    
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

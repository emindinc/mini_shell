#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024

int main() {
    char command[MAX_CMD_LEN];
    
    printf("Mini Shell başlatıldı. Çıkmak için 'exit' yazın.\n");
    
    while(1) {
        // Prompt göster
        printf("myshell> ");
        fflush(stdout);
        
        // Komut oku
        if(fgets(command, MAX_CMD_LEN, stdin) == NULL) {
            break;
        }
        
        // Newline'ı temizle
        command[strcspn(command, "\n")] = 0;
        
        // Exit kontrolü
        if(strcmp(command, "exit") == 0) {
            printf("Çıkılıyor...\n");
            break;
        }
        
        // Boş komut kontrolü
        if(strlen(command) == 0) {
            continue;
        }
        
        printf("Girilen komut: %s\n", command);
        // Şimdilik sadece yazdır, sonra çalıştıracağız
    }
    
    return 0;
}

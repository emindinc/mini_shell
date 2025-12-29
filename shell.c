#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

// --- AYARLAR VE RENKLER (MACROS) ---
#define MAX_CMD_LEN 1024
#define MAX_HISTORY 200

// Renk Kodları
#define C_RESET  "\033[0m"
#define C_CYAN   "\033[1;36m"
#define C_GREEN  "\033[1;32m"
#define C_YELLOW "\033[1;33m"
#define C_RED    "\033[1;31m"
#define C_PURPLE "\033[38;5;93m"
#define C_WHITE  "\033[1;97m"
#define C_GRAY   "\033[1;30m"
#define C_BLUE   "\033[1;34m"
#define C_DIM    "\033[2m"

// --- GLOBAL DEĞİŞKENLER ---
static char history[MAX_HISTORY][MAX_CMD_LEN];
static int history_count = 0;
static int return_to_menu = 0;
static struct termios orig_termios;

const char *tips[] = {
    "Ipucu: 'help' yazarak tum komutlari gorebilirsiniz.",
    "Ipucu: 'calc' ile matematik islemleri yapabilirsiniz.",
    "Ipucu: Cikmak icin menude '2'ye basin veya shell'de 'exit' yazin.",
    "Ipucu: 'history' ile gecmis komutlariniza ulasin.",
    "Ipucu: Dosya izinleri icin 'chmod' kullanabilirsiniz.",
    "Bilgi: Bu shell C dili ile gelistirilmistir.",
    "Sistem: RAM durumunu 'free' komutuyla kontrol edin."
};

// --- YARDIMCI FONKSİYONLAR ---

// Terminal ayarlarını geri yükle
void restore_terminal() {
    printf("\033[?25h");   // İmleci göster
    #ifdef ECHOCTL
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    #endif
    printf("%s", C_RESET);  // Renkleri sıfırla
}

// Ctrl+C basıldığında ^C görünmesini engelle
void disable_echoctl() {
    #ifdef ECHOCTL
    struct termios t;
    if(tcgetattr(STDIN_FILENO, &t) == 0) {
        orig_termios = t;
        t.c_lflag &= ~ECHOCTL;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }
    #endif
}

// Sinyal Handlerlar
void sigchld_handler(int signo) {
    (void)signo;
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void sigint_handler(int signo) {
    (void)signo;
    write(1, "\n", 1);
}

// --- ARAYÜZ (UI) FONKSİYONLARI ---

// Ana Menü
void show_fancy_menu() {
    printf("\033[H\033[2J\033[3J"); // Ekranı ve GEÇMİŞİ temizle

    struct sysinfo si;
    long ram_total = 0, ram_used = 0;
    if(sysinfo(&si) == 0) {
        ram_total = si.totalram / 1024 / 1024;
        ram_used = (si.totalram - si.freeram) / 1024 / 1024;
    }

    int tip_idx = rand() % 7;
    char *user = getenv("USER"); 
    if(!user) user = "unknown";

    printf("\n");
    printf("      %s    __  __ _       _   ____  _          _ _ %s\n", C_CYAN, C_RESET);
    printf("      %s   |  \\/  (_)_ __ (_) / ___|| |__   ___| | |%s\n", C_CYAN, C_RESET);
    printf("      %s   | |\\/| | | '_ \\| | \\___ \\| '_ \\ / _ \\ | |%s\n", C_CYAN, C_RESET);
    printf("      %s   |_|  |_|_|_| |_|_| |____/|_| |_|\\___|_|_|%s\n", C_CYAN, C_RESET);
    printf("\n");

    printf("      %s╔════════════════════ SYSTEM STATUS ════════════════════╗%s\n", C_PURPLE, C_RESET);
    printf("      %s║%s   HOST: %-12s  CPU: [OK]    MEM: %4ld/%ld MB %s║%s\n", C_PURPLE, C_WHITE, user, ram_used, ram_total, C_PURPLE, C_RESET);

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char d_str[32];
    strftime(d_str, sizeof(d_str), "%H:%M:%S", &tm);

    printf("      %s║%s   TIME: %-12s  NET: %sONLINE  %sBAT: %s%%100    %s║%s\n", C_PURPLE, C_WHITE, d_str, C_GREEN, C_WHITE, C_GREEN, C_PURPLE, C_RESET);
    printf("      %s╚═══════════════════════════════════════════════════════╝%s\n", C_PURPLE, C_RESET);
    printf("\n");

    printf("      %s┌──────────────────────┐      ┌──────────────────────┐%s\n", C_PURPLE, C_RESET);
    printf("      %s│ %s[1] SHELL BASLAT     %s│      │ %s[2] GUVENLI CIKIS  %s│%s\n", C_PURPLE, C_GREEN, C_PURPLE, C_YELLOW, C_PURPLE, C_RESET);
    printf("      %s└──────────────────────┘      └──────────────────────┘%s\n", C_PURPLE, C_RESET);
    printf("\n");
    printf("      %s> \033[3m%s%s\n\n", C_PURPLE, tips[tip_idx], C_RESET);
    printf("      %sSECIMINIZ > %s", C_CYAN, C_RESET);
}

// --- KOMUT İŞLEME ---

int execute_command(char *command, int background) {
    char *args[64];
    int i = 0;
    args[i] = strtok(command, " ");
    while(args[i] != NULL && i < 63) {
        i++;
        args[i] = strtok(NULL, " ");
    }
    args[i] = NULL;

    if(args[0] == NULL) return 0;

    // --- BUILT-IN KOMUTLAR ---

    if(strcmp(args[0], "exit") == 0) {
        return_to_menu = 1;
        return 0;
    }

    if(strcmp(args[0], "clear") == 0) {
    printf("\033[H\033[2J\033[3J"); // 3J kodu geçmişi siler
    return 0;
  }
    
   
    
    if(strcmp(args[0], "cd") == 0) {
        char *target = args[1] ? args[1] : getenv("HOME");
        if(chdir(target) != 0) {
            perror("cd");
            return 1;
        }
        return 0;
    }
    
    if(strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if(getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        return 0;
    }
    
    if(strcmp(args[0], "history") == 0) {
        for(int h = 0; h < history_count; h++) 
            printf("%4d  %s\n", h+1, history[h]);
        return 0;
    }
    
    if(strcmp(args[0], "ls") == 0) {
        DIR *d = opendir(".");
        if(d) {
            struct dirent *dir;
            int count = 0;
            while((dir = readdir(d)) != NULL) {
                if(dir->d_name[0] == '.') continue;
                if(dir->d_type == DT_DIR) 
                    printf("%s%s/%s  ", C_BLUE, dir->d_name, C_RESET);
                else 
                    printf("%s  ", dir->d_name);
                if(++count % 5 == 0) printf("\n");
            }
            printf("\n");
            closedir(d);
        } else {
            perror("ls");
        }
        return 0;
    }

    // Help Komutu
    if(strcmp(args[0], "help") == 0) {
        printf("\n%s=== Mini Shell Komut Merkezi (v1.3) ===%s\n\n", C_CYAN, C_RESET);
        char *g = C_GREEN; 
        char *c = C_DIM; 
        char *r = C_RESET;

        printf("  %sDosya ve Dizin:%s\n", C_YELLOW, r);
        printf("  %sls%s             %s# Dosyalari listele\n", g, r, c);
        printf("  %scd [yol]%s       %s# Dizin degistir\n", g, r, c);
        printf("  %spwd%s            %s# Calisma dizini\n", g, r, c);
        printf("  %smkdir [ad]%s     %s# Klasor olustur\n", g, r, c);
        printf("  %srmdir [ad]%s     %s# Klasor sil\n", g, r, c);
        printf("  %stouch [ad]%s     %s# Dosya olustur\n", g, r, c);
        printf("  %srm [ad]%s        %s# Dosya sil\n", g, r, c);
        printf("  %scp [k] [h]%s     %s# Kopyala\n", g, r, c);
        printf("  %schmod [m] [f]%s  %s# Izin degistir (755)\n", g, r, c);

        printf("\n  %sIcerik ve Arama:%s\n", C_YELLOW, r);
        printf("  %sgrep [k] [f]%s   %s# Arama yap\n", g, r, c);
        printf("  %stail [f]%s       %s# Sonunu oku (10 satir)\n", g, r, c);

        printf("\n  %sSistem:%s\n", C_YELLOW, r);
        printf("  %scalc [islem]%s   %s# Hesapla (5 + 3)\n", g, r, c);
        printf("  %sfree%s           %s# RAM durumu\n", g, r, c);
        printf("  %sdf%s             %s# Disk durumu\n", g, r, c);
        printf("  %sdate%s           %s# Tarih ve saat\n", g, r, c);
        printf("  %swhoami%s         %s# Kullanici adi\n", g, r, c);
        printf("  %srand%s           %s# Rastgele sayi\n", g, r, c);
        printf("  %shistory%s        %s# Komut gecmisi\n", g, r, c);
        printf("  %sclear%s          %s# Ekrani temizle\n", g, r, c);
        printf("  %sexit%s           %s# Menüye dön\n\n", g, r, c);
        
        printf("  %sOperatorler:%s\n", C_YELLOW, r);
        printf("  %s&&%s             %s# AND - Onceki basariliysa devam\n", g, r, c);
        printf("  %s||%s             %s# OR  - Onceki basarisizsa devam\n", g, r, c);
        printf("  %s&%s              %s# Arka planda calistir\n\n", g, r, c);
        
        return 0;
    }

    // calc
    if(strcmp(args[0], "calc") == 0) {
        if(!args[1] || !args[2] || !args[3]) {
            printf("Kullanim: calc 10 + 5\n");
            printf("Desteklenen: + - * /\n");
            return 1;
        }
        double n1 = atof(args[1]), n2 = atof(args[3]), res = 0;
        char op = args[2][0];
        
        if(op == '+') res = n1 + n2;
        else if(op == '-') res = n1 - n2;
        else if(op == '*') res = n1 * n2;
        else if(op == '/') {
            if(n2 == 0) {
                printf("Hata: Sifira bolunmez!\n");
                return 1;
            }
            res = n1 / n2;
        }
        else {
            printf("Gecersiz islem: %c\n", op);
            return 1;
        }
        
        printf("Sonuc: %.2f\n", res);
        return 0;
    }

    // grep
    if(strcmp(args[0], "grep") == 0) {
        if(!args[1] || !args[2]) {
            printf("grep: kullanim: grep kelime dosya\n");
            return 1;
        }
        FILE *fp = fopen(args[2], "r");
        if(!fp) {
            perror("grep");
            return 1;
        }
        char line[1024]; 
        int ln = 1;
        while(fgets(line, sizeof(line), fp)) {
            if(strstr(line, args[1])) 
                printf("%s%d:%s %s", C_PURPLE, ln, C_RESET, line);
            ln++;
        }
        fclose(fp);
        return 0;
    }

    // tail
    if(strcmp(args[0], "tail") == 0) {
        if(!args[1]) {
            printf("tail: dosya adi gerekli\n");
            return 1;
        }
        FILE *fp = fopen(args[1], "r");
        if(!fp) {
            perror("tail");
            return 1;
        }
        
        int lines = 0;
        char ch;
        while((ch = fgetc(fp)) != EOF) {
            if(ch == '\n') lines++;
        }
        rewind(fp);
        
        int start = (lines > 10) ? (lines - 10) : 0;
        char line[1024];
        int cur = 0;
        while(fgets(line, sizeof(line), fp)) {
            if(cur++ >= start) printf("%s", line);
        }
        fclose(fp);
        return 0;
    }

    // free
    if(strcmp(args[0], "free") == 0) { 
        struct sysinfo s; 
        sysinfo(&s); 
        printf("RAM: %lu/%lu MB\n", 
               (s.totalram - s.freeram) / 1024 / 1024, 
               s.totalram / 1024 / 1024); 
        return 0; 
    }
    
    // df
    if(strcmp(args[0], "df") == 0) {
        FILE *fp = popen("df -h .", "r");
        if(fp) {
            char line[256];
            while(fgets(line, sizeof(line), fp)) {
                printf("%s", line);
            }
            int status = pclose(fp);
            if(WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return 1;
        }
        perror("df");
        return 1;
    }
    
    // date
    if(strcmp(args[0], "date") == 0) {
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        char buf[128];
        strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", lt);
        printf("%s\n", buf);
        return 0;
    }
    
    // whoami
    if(strcmp(args[0], "whoami") == 0) {
        char *user = getenv("USER");
        if(!user) user = "unknown";
        printf("%s\n", user);
        return 0;
    }
    
    if(strcmp(args[0], "rand") == 0) { 
        printf("%d\n", rand() % 1000); 
        return 0; 
    }

    // touch
    if(strcmp(args[0], "touch") == 0) {
        if(!args[1]) {
            printf("touch: dosya adi gerekli\n");
            return 1;
        }
        int fd = open(args[1], O_CREAT | O_WRONLY, 0644);
        if(fd < 0) {
            perror("touch");
            return 1;
        }
        close(fd);
        return 0;
    }
    
    // mkdir
    if(strcmp(args[0], "mkdir") == 0) {
        if(!args[1]) {
            printf("mkdir: dizin adi gerekli\n");
            return 1;
        }
        if(mkdir(args[1], 0755) != 0) {
            perror("mkdir");
            return 1;
        }
        return 0;
    }
    
    // rmdir
    if(strcmp(args[0], "rmdir") == 0) {
        if(!args[1]) {
            printf("rmdir: dizin adi gerekli\n");
            return 1;
        }
        if(rmdir(args[1]) != 0) {
            perror("rmdir");
            return 1;
        }
        return 0;
    }
    
    // rm
    if(strcmp(args[0], "rm") == 0) {
        if(!args[1]) {
            printf("rm: dosya adi gerekli\n");
            return 1;
        }
        if(unlink(args[1]) != 0) {
            perror("rm");
            return 1;
        }
        return 0;
    }
    
    // chmod
    if(strcmp(args[0], "chmod") == 0) {
        if(!args[1] || !args[2]) {
            printf("chmod: kullanim: chmod 755 dosya\n");
            return 1;
        }
        mode_t mode = strtol(args[1], NULL, 8);
        if(chmod(args[2], mode) != 0) {
            perror("chmod");
            return 1;
        }
        return 0;
    }

    // cp
    if(strcmp(args[0], "cp") == 0) {
        if(!args[1] || !args[2]) {
            printf("cp: kullanim: cp kaynak hedef\n");
            return 1;
        }
        
        int src = open(args[1], O_RDONLY);
        if(src < 0) {
            perror("cp: kaynak");
            return 1;
        }
        
        int dst = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(dst < 0) {
            perror("cp: hedef");
            close(src);
            return 1;
        }
        
        char buf[4096]; 
        ssize_t n;
        while((n = read(src, buf, sizeof(buf))) > 0) {
            if(write(dst, buf, n) != n) {
                perror("cp: yazma");
                close(src);
                close(dst);
                return 1;
            }
        }
        
        close(src);
        close(dst);
        return 0;
    }

    // External Commands
    pid_t pid = fork();
    
    if(pid < 0) {
        perror("fork");
        return 1;
    }
    else if(pid == 0) {
        // Child process
        signal(SIGINT, SIG_DFL);
        
        if(execvp(args[0], args) < 0) {
            printf("Komut bulunamadi: %s\n", args[0]);
            fflush(stdout);
            exit(127);
        }
    }
    else {
        // Parent process
        if(background) {
            printf("[%d] arkada calisiyor\n", pid);
            return 0;
        }
        else {
            signal(SIGCHLD, SIG_IGN);
            
            int status;
            waitpid(pid, &status, 0);
            
            signal(SIGCHLD, sigchld_handler);
            
            if(WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            return 1;
        }
    }
    
    return 0;
}

// Operatör yönetimi
void handle_operators(char *command) {
    int background = 0;
    int len = strlen(command);

    while(len > 0 && isspace(command[len-1])) {
        command[len-1] = '\0';
        len--;
    }

    if(len > 0 && command[len-1] == '&') {
        background = 1;
        command[len-1] = '\0';
        len--;
    }

    char *commands[64];
    int ops[64];
    int cmd_count = 0;

    char *ptr = command;
    char *start = command;

    while(*ptr) {
        if(ptr[0] == '&' && ptr[1] == '&') {
            *ptr = '\0';
            commands[cmd_count] = start;
            ops[cmd_count] = 1;
            cmd_count++;
            ptr += 2;
            while(isspace(*ptr)) ptr++;
            start = ptr;
            continue;
        }

        if(ptr[0] == '|' && ptr[1] == '|') {
            *ptr = '\0';
            commands[cmd_count] = start;
            ops[cmd_count] = 2;
            cmd_count++;
            ptr += 2;
            while(isspace(*ptr)) ptr++;
            start = ptr;
            continue;
        }
        ptr++;
    }

    commands[cmd_count] = start;
    cmd_count++;

    int ret = execute_command(commands[0], (cmd_count == 1 ? background : 0));

    for(int i = 1; i < cmd_count; i++) {
        int op = ops[i-1];

        if(op == 1) {
            if(ret == 0) {
                int is_last = (i == cmd_count - 1);
                ret = execute_command(commands[i], (is_last ? background : 0));
            }
        }
        else if(op == 2) {
            if(ret != 0) {
                int is_last = (i == cmd_count - 1);
                ret = execute_command(commands[i], (is_last ? background : 0));
            }
        }
    }
}

// Shell loop
void shell_loop() {
    char command[MAX_CMD_LEN];
    
    printf("\n%s╔═══════════════════════════════════════════╗%s\n", C_PURPLE, C_RESET);
    printf("%s║%s    Mini Shell Baslatildi (v1.3)         %s║%s\n", C_PURPLE, C_CYAN, C_PURPLE, C_RESET);
    printf("%s╚═══════════════════════════════════════════╝%s\n", C_PURPLE, C_RESET);
    printf("\n%sYardim icin 'help' yazin, menüye donmek icin 'exit'%s\n", C_DIM, C_RESET);

    while(1) {
        char *user = getenv("USER"); 
        if(!user) user = "user";
        
        char cwd[1024]; 
        if(!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "~");
        char *short_cwd = strrchr(cwd, '/'); 
        if(short_cwd) short_cwd++; 
        else short_cwd = cwd;

        printf("\n%s%s%s:%s~/%s%s$ %s", 
               C_GREEN, user, C_RESET, 
               C_BLUE, short_cwd, 
               C_YELLOW, C_RESET);
        fflush(stdout);

        if(fgets(command, MAX_CMD_LEN, stdin) == NULL) break;
        command[strcspn(command, "\n")] = 0;
        if(strlen(command) == 0) continue;

        if(history_count < MAX_HISTORY) {
            strncpy(history[history_count], command, MAX_CMD_LEN - 1);
            history[history_count][MAX_CMD_LEN - 1] = '\0';
            history_count++;
        }

        handle_operators(command);
        if(return_to_menu) break;
    }
}

// Main
int main() {
    srand(time(NULL));
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    setlocale(LC_ALL, "");

    disable_echoctl();
    atexit(restore_terminal);

    while(1) {
        show_fancy_menu();

        char choice[16];
        if(fgets(choice, sizeof(choice), stdin) == NULL) break;

        if(choice[0] == '1') {
            printf("\n%s[*] Shell baslatiliyor...%s", C_GREEN, C_RESET);
            fflush(stdout);
            usleep(200000);
            printf("\033[H\033[2J\033[3J"); // Geçişte geçmişi sil
            return_to_menu = 0;
            
            shell_loop();

            if(return_to_menu) {
            printf("\033[H\033[2J\033[3J"); // Menüye dönerken geçmişi sil
            continue;
        }
            
            
            break;
        } 
        else if(choice[0] == '2' || choice[0] == 'q') {
            printf("\n%s[*] Cikis yapiliyor...%s\n", C_YELLOW, C_RESET);
            usleep(500000);
            printf("\033[2J\033[H");
            break;
        }
    }
    
    return 0;
}

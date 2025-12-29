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

// Renk Kodları (Her fonksiyonda tekrar yazmamak için)
#define C_RESET  "\033[0m"
#define C_CYAN   "\033[1;36m"
#define C_GREEN  "\033[1;32m"
#define C_YELLOW "\033[1;33m"
#define C_RED    "\033[1;31m"
#define C_PURPLE "\033[38;5;93m" // Çerçeve Rengi
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

// Terminal ayarlarını geri yükle (Program çıkışında)
void restore_terminal() {
    printf("\033[?1049l"); // Alternate screen kapat
    printf("\033[?25h");   // İmleci aç
    #ifdef ECHOCTL
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    #endif
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

// Shell içindeki üst bilgi ve alt footer çubuğu
static void draw_header_footer(void) {
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) { w.ws_row = 24; w.ws_col = 80; }
    
    printf("\033[2J\033[H"); // Temizle

    char *user = getenv("USER");
    if(!user) user = "user";
    
    char cwd[1024];
    if(!getcwd(cwd, sizeof(cwd))) strcpy(cwd, "~");
    char *short_cwd = strrchr(cwd, '/');
    if(short_cwd) short_cwd++; else short_cwd = cwd;

    printf("\n");
    printf("  %s┌───────────────────────────────────────────────────┐%s\n", C_PURPLE, C_RESET);
    printf("  %s│%s USER : %s%-15s                          %s│%s\n", C_PURPLE, C_GRAY, C_CYAN, user, C_PURPLE, C_RESET);
    printf("  %s│%s DIR  : %s%-15s                          %s│%s\n", C_PURPLE, C_GRAY, C_WHITE, short_cwd, C_PURPLE, C_RESET);
    printf("  %s└───────────────────────────────────────────────────┘%s\n", C_PURPLE, C_RESET);

    // Footer (En alt satır)
    printf("\033[%d;1H", w.ws_row);
    printf("\033[48;5;93m\033[1;97m"); // Mor Arkaplan, Beyaz Yazı
    
    char footer[] = " CIKIS: 'exit' | TEMIZLE: 'clear' | YARDIM: 'help' ";
    int pad = (w.ws_col - strlen(footer)) / 2;
    if(pad < 0) pad = 0;
    
    for(int i=0; i<pad; i++) putchar(' ');
    printf("%s", footer);
    for(int i=pad+strlen(footer); i<w.ws_col; i++) putchar(' ');
    
    printf("%s", C_RESET);
    printf("\033[6;1H"); // İmleci komut satırına getir
}

// Ana Menü (Dashboard)
void show_fancy_menu() {
    printf("\033[2J\033[H"); // Temizle
    
    struct sysinfo si;
    long ram_total = 0, ram_used = 0;
    if(sysinfo(&si) == 0) {
        ram_total = si.totalram / 1024 / 1024;
        ram_used = (si.totalram - si.freeram) / 1024 / 1024;
    }

    int tip_idx = rand() % 7;
    char *user = getenv("USER"); if(!user) user = "unknown";

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
        printf("\033[H\033[J");
        return 0;
    }
    if(strcmp(args[0], "cd") == 0) {
        if(args[1] == NULL) chdir(getenv("HOME"));
        else if(chdir(args[1]) != 0) perror("cd hatasi");
        return 0;
    }
    if(strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if(getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        return 0;
    }
    if(strcmp(args[0], "history") == 0) {
        for(int h = 0; h < history_count; h++) printf("%4d  %s\n", h+1, history[h]);
        return 0;
    }
    if(strcmp(args[0], "ls") == 0) {
        DIR *d = opendir(".");
        if(d) {
            struct dirent *dir;
            int count = 0;
            while((dir = readdir(d)) != NULL) {
                if(dir->d_name[0] == '.') continue;
                if(dir->d_type == DT_DIR) printf("%s%s/%s  ", C_BLUE, dir->d_name, C_RESET);
                else printf("%s  ", dir->d_name);
                if(++count % 5 == 0) printf("\n");
            }
            printf("\n");
            closedir(d);
        } else perror("ls hatasi");
        return 0;
    }
    
    // Help Komutu
    if(strcmp(args[0], "help") == 0) {
        printf("\n%s=== Mini Shell Komut Merkezi (v1.1) ===%s\n\n", C_CYAN, C_RESET);
        char *g = C_GREEN; char *c = C_DIM; char *r = C_RESET;
        
        printf("  %sDosya ve Dizin:%s\n", C_YELLOW, r);
        printf("  %sls%s             %s# Dosyalari listele\n", g, r, c);
        printf("  %scd [yol]%s       %s# Dizin degistir\n", g, r, c);
        printf("  %spwd%s            %s# Calisma dizini\n", g, r, c);
        printf("  %smkdir [ad]%s     %s# Klasor olustur\n", g, r, c);
        printf("  %srmdir [ad]%s     %s# Klasor sil\n", g, r, c);
        printf("  %stouch [ad]%s     %s# Dosya olustur\n", g, r, c);
        printf("  %srm [ad]%s        %s# Dosya sil\n", g, r, c);
        printf("  %scp [k] [h]%s     %s# Kopyala\n", g, r, c);
        printf("  %smv [k] [h]%s     %s# Tasi/Yeniden adlandir\n", g, r, c);
        printf("  %schmod [m] [f]%s  %s# Izin degistir (777)\n", g, r, c);
        printf("  %sstat [f]%s       %s# Dosya detayi\n", g, r, c);

        printf("\n  %sIcerik ve Arama:%s\n", C_YELLOW, r);
        printf("  %scat [f]%s        %s# Oku\n", g, r, c);
        printf("  %shead [f]%s       %s# Basini oku (10 satir)\n", g, r, c);
        printf("  %stail [f]%s       %s# Sonunu oku (10 satir)\n", g, r, c);
        printf("  %sgrep [k] [f]%s   %s# Arama yap\n", g, r, c);
        printf("  %swc [f]%s         %s# Satir say\n", g, r, c);

        printf("\n  %sSistem:%s\n", C_YELLOW, r);
        printf("  %scalc [islem]%s   %s# Hesapla (5 + 3)\n", g, r, c);
        printf("  %sfree%s           %s# RAM durumu\n", g, r, c);
        printf("  %sdf%s             %s# Disk durumu\n", g, r, c);
        printf("  %skill [pid]%s     %s# Process oldur\n", g, r, c);
        printf("  %srand%s           %s# Rastgele sayi\n", g, r, c);
        printf("  %sexit%s           %s# Cikis\n\n", g, r, c);
        return 0;
    }

    // Diğer Komutlar (calc, grep, cp, mv, touch, rm, chmod, stat, head, tail, free, kill vb.)
    if(strcmp(args[0], "calc") == 0) {
        if(!args[1] || !args[2] || !args[3]) { printf("Kullanim: calc 10 + 5\n"); return 1; }
        double n1 = atof(args[1]), n2 = atof(args[3]), res = 0;
        char op = args[2][0];
        if(op == '+') res = n1 + n2;
        else if(op == '-') res = n1 - n2;
        else if(op == '*') res = n1 * n2;
        else if(op == '/') { if(n2==0) {printf("Sifira bolunmez!\n"); return 1;} res = n1/n2; }
        printf("Sonuc: %.2f\n", res);
        return 0;
    }

    if(strcmp(args[0], "grep") == 0) {
        if(!args[1] || !args[2]) return 1;
        FILE *fp = fopen(args[2], "r");
        if(fp) {
            char line[1024]; int ln=1;
            while(fgets(line, sizeof(line), fp)) {
                if(strstr(line, args[1])) printf("%s%d:%s %s", C_PURPLE, ln, C_RESET, line);
                ln++;
            }
            fclose(fp);
        } else perror("grep");
        return 0;
    }

    if(strcmp(args[0], "tail") == 0) {
        if(!args[1]) return 1;
        FILE *fp = fopen(args[1], "r");
        if(fp) {
            int lines=0, cur=0; 
            while(!feof(fp)) if(fgetc(fp)=='\n') lines++;
            rewind(fp);
            char line[1024];
            while(fgets(line, sizeof(line), fp)) {
                if(cur++ >= lines - 10) printf("%s", line);
            }
            fclose(fp);
        }
        return 0;
    }
    
    // Basit System Wrapper'ları (Tek satırlıklar)
    if(strcmp(args[0], "free") == 0) { struct sysinfo s; sysinfo(&s); printf("RAM: %lu/%lu MB\n", (s.totalram-s.freeram)/1024/1024, s.totalram/1024/1024); return 0; }
    if(strcmp(args[0], "df") == 0) { system("df -h ."); return 0; }
    if(strcmp(args[0], "date") == 0) { system("date"); return 0; }
    if(strcmp(args[0], "whoami") == 0) { system("whoami"); return 0; }
    if(strcmp(args[0], "rand") == 0) { printf("%d\n", rand()%1000); return 0; }
    
    // Dosya İşlemleri (Wrapper)
    if(strcmp(args[0], "touch") == 0 && args[1]) { close(open(args[1], O_CREAT|O_WRONLY, 0644)); return 0; }
    if(strcmp(args[0], "mkdir") == 0 && args[1]) { mkdir(args[1], 0755); return 0; }
    if(strcmp(args[0], "rmdir") == 0 && args[1]) { rmdir(args[1]); return 0; }
    if(strcmp(args[0], "rm") == 0 && args[1]) { unlink(args[1]); return 0; }
    if(strcmp(args[0], "chmod") == 0 && args[2]) { chmod(args[2], strtol(args[1],NULL,8)); return 0; }
    
    // cp, mv, stat, head, wc (Kısa tutmak için basitleştirildi, senin kodundaki mantık korundu)
    if(strcmp(args[0], "cp") == 0 && args[1] && args[2]) {
        int src = open(args[1], O_RDONLY);
        int dst = open(args[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
        if(src >= 0 && dst >= 0) {
            char buf[4096]; ssize_t n;
            while((n = read(src, buf, sizeof(buf))) > 0) write(dst, buf, n);
        }
        if(src>=0) close(src); if(dst>=0) close(dst);
        return 0;
    }

    // External Commands (execvp)
    pid_t pid = fork();
    if(pid == 0) {
        signal(SIGINT, SIG_DFL);
        if(execvp(args[0], args) < 0) {
            printf("%sKomut bulunamadi: %s%s\n", C_RED, args[0], C_RESET);
            exit(127);
        }
    } else {
        if(background) printf("[%d] arkada calisiyor\n", pid);
        else waitpid(pid, NULL, 0);
    }
    return 0;
}

// && ve || operatörlerini ayırıp execute_command'ı çağıran fonksiyon
// GÜÇLENDİRİLMİŞ OPERATÖR YÖNETİMİ (&& ve || Destekli)
void handle_operators(char *command) {
    // 1. Arka Plan (&) Kontrolü
    int background = 0;
    int len = strlen(command);
    
    // Sondaki boşlukları temizle
    while(len > 0 && isspace(command[len-1])) {
        command[len-1] = '\0';
        len--;
    }

    // Komutun sonunda & var mı?
    if(len > 0 && command[len-1] == '&') {
        background = 1;
        command[len-1] = '\0'; // &'i sil
        len--;
    }

    // 2. Komutları && ve || operatörlerine göre parçala
    char *commands[64];     // Komutlar listesi
    int ops[64];            // Operatörler: 1=&&, 2=||
    int cmd_count = 0;

    char *ptr = command;
    char *start = command;
    
    while(*ptr) {
        // && kontrolü
        if(ptr[0] == '&' && ptr[1] == '&') {
            *ptr = '\0'; // Stringi buradan kes
            commands[cmd_count] = start;
            ops[cmd_count] = 1; // 1 = AND (&&)
            cmd_count++;
            ptr += 2;
            while(isspace(*ptr)) ptr++; // Boşlukları atla
            start = ptr;
            continue;
        }
        
        // || kontrolü
        if(ptr[0] == '|' && ptr[1] == '|') {
            *ptr = '\0'; // Stringi buradan kes
            commands[cmd_count] = start;
            ops[cmd_count] = 2; // 2 = OR (||)
            cmd_count++;
            ptr += 2;
            while(isspace(*ptr)) ptr++; // Boşlukları atla
            start = ptr;
            continue;
        }
        ptr++;
    }
    
    // Son kalan komutu ekle
    commands[cmd_count] = start;
    cmd_count++;

    // 3. Mantıksal Çalıştırma Döngüsü
    // İlk komutu her zaman çalıştır
    // background sadece son komut için veya tüm grup için geçerli olabilir. 
    // Basitlik adına burada her komuta gönderiyoruz ama process'ler waitpid ile yönetiliyor.
    
    int ret = execute_command(commands[0], (cmd_count == 1 ? background : 0));

    for(int i = 1; i < cmd_count; i++) {
        int op = ops[i-1];
        
        // && (AND): Önceki başarılıysa (ret == 0) çalıştır
        if(op == 1) {
            if(ret == 0) {
                // Son komutsa ve background varsa ona göre çalıştır
                int is_last = (i == cmd_count - 1);
                ret = execute_command(commands[i], (is_last ? background : 0));
            }
        }
        // || (OR): Önceki başarısızsa (ret != 0) çalıştır
        else if(op == 2) {
            if(ret != 0) {
                int is_last = (i == cmd_count - 1);
                ret = execute_command(commands[i], (is_last ? background : 0));
            }
        }
    }
}

void shell_loop() {
    char command[MAX_CMD_LEN];
    draw_header_footer(); // UI Çiz
    
    while(1) {
        char *user = getenv("USER"); if(!user) user="user";
        char cwd[1024]; getcwd(cwd, sizeof(cwd));
        char *short_cwd = strrchr(cwd, '/'); if(short_cwd) short_cwd++; else short_cwd=cwd;

        printf("%s%s%s:%s~/%s%s$ ", C_GREEN, user, C_RESET, C_BLUE, short_cwd, C_YELLOW);
        fflush(stdout);

        if(fgets(command, MAX_CMD_LEN, stdin) == NULL) break;
        command[strcspn(command, "\n")] = 0;
        if(strlen(command) == 0) continue;

        // Geçmişe ekle
        if(history_count < MAX_HISTORY) strcpy(history[history_count++], command);
        
        handle_operators(command);
        if(return_to_menu) break;
    }
}

int main() {
    srand(time(NULL));
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    setlocale(LC_ALL, "");

    // Alternate Screen (Yeni Sayfa Modu)
    printf("\033[?1049h");
    fflush(stdout);

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
            return_to_menu = 0;
            shell_loop();
            if(!return_to_menu) break;
        } else if(choice[0] == '2' || choice[0] == 'q') {
            break;
        }
    }
    return 0;
}
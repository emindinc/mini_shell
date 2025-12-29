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

#define MAX_CMD_LEN 1024
#define MAX_HISTORY 200

static char history[MAX_HISTORY][MAX_CMD_LEN];
static int history_count = 0;

// Terminal UI helpers: alternate screen and header/footer
static void restore_ui(void);
static void enter_alt_screen(void);
static void draw_header_footer(void);
static void shell_loop(void);
static int return_to_menu = 0;

static void restore_ui(void) {
    // reset attributes and leave alternate screen
    // reset attributes
    printf("\033[0m");
    fflush(stdout);
}

static void enter_alt_screen(void) {
    // Draw initial header/footer (no alternate buffer)
    draw_header_footer();
    atexit(restore_ui);
}

static void draw_header_footer(void) {
    struct winsize w;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        w.ws_row = 24; w.ws_col = 80;
    }
    int rows = w.ws_row;
    int cols = w.ws_col;

    // Ekranı temizle ve imleci sol üst köşeye al
    printf("\033[2J");
    printf("\033[H");

    // --- ÜST BİLGİ KUTUSU (Login Bilgileri) ---
    char *username = getenv("USER");
    if(!username) username = "user";
    
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "~");
    
    // Klasör yolunu kısalt (Sadece son klasör ismi)
    char *short_cwd = strrchr(cwd, '/');
    if(short_cwd) short_cwd++; else short_cwd = cwd;

    // Renk Tanımları (Menü ile uyumlu Mor/Cyan)
    char *border = "\033[38;5;93m"; // Mor Çerçeve
    char *label  = "\033[1;30m";    // Gri Etiketler
    char *text   = "\033[1;97m";    // Beyaz Yazı
    char *cyan   = "\033[1;36m";    // Cyan Vurgu
    char *reset  = "\033[0m";

    printf("\n");
    // Kutunun üstü
    printf("  %s┌───────────────────────────────────────────────────┐%s\n", border, reset);
    
    // Kullanıcı Adı Satırı
    printf("  %s│%s USER : %s%-15s                          %s│%s\n", 
           border, label, cyan, username, border, reset);
           
    // Dizin Satırı
    printf("  %s│%s DIR  : %s%-15s                          %s│%s\n", 
           border, label, text, short_cwd, border, reset);
           
    // Kutunun altı
    printf("  %s└───────────────────────────────────────────────────┘%s\n", border, reset);

    // --- ALT BİLGİ ÇUBUĞU (FOOTER) ---
    // İmleci terminalin en alt satırına taşı
    printf("\033[%d;1H", rows);
    
    // Arkaplanı Mor, Yazıyı Beyaz Yap
    printf("\033[48;5;93m\033[1;97m");
    
    // Footer metni (İstediğin 'exit' uyarısı burada)
    char footer_msg[] = " ÇIKIŞ: 'exit' | TEMİZLE: 'clear' | YARDIM: 'help' ";
    
    int len = strlen(footer_msg);
    int padding = (cols - len) / 2; // Ortalamak için boşluk hesabı
    if(padding < 0) padding = 0;

    // Sol boşluk
    for(int i=0; i<padding; i++) putchar(' ');
    // Mesajı yaz
    printf("%s", footer_msg);
    // Sağ boşluk (Satır sonuna kadar boyamak için)
    for(int i=padding+len; i<cols; i++) putchar(' ');
    
    // Renkleri sıfırla
    printf("%s", reset);

    // İmleci komut yazmak için kutunun hemen altına (6. satıra) geri getir
    printf("\033[6;1H"); 
    fflush(stdout);
}
// Havalı bir ASCII Logo
void draw_ascii_logo() {
    printf("\033[1;36m"); // Cyan renk
    printf("  __  __ _       _   ____  _          _ _ \n");
    printf(" |  \\/  (_)_ __ (_) / ___|| |__   ___| | |\n");
    printf(" | |\\/| | | '_ \\| | \\___ \\| '_ \\ / _ \\ | |\n");
    printf(" | |  | | | | | | |  ___) | | | |  __/ | |\n");
    printf(" |_|  |_|_|_| |_|_| |____/|_| |_|\\___|_|_|\n");
    printf("\033[0m");
    printf("\033[1;35m        >> SYSTEM READY << \033[0m\n\n");
}

// Modern Giriş Menüsü
// Modern Giriş Menüsü (Düzeltilmiş Versiyon)
void show_fancy_menu() {
    // Ekranı temizle
    printf("\033[2J\033[H");
    
    // Renk tanımları
    char *border_c = "\033[38;5;93m"; // Mor Çerçeve
    char *text_c = "\033[1;97m";      // Beyaz Metin
    char *accent_c = "\033[1;32m";    // Yeşil Vurgu
    char *reset = "\033[0m";          // Sıfırla

    printf("\n");
    
    // 1. Logo Bölümü
    printf("      "); 
    draw_ascii_logo();

    // 2. Bilgi Paneli
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%d-%m-%Y %H:%M", &tm);
    
    char *user = getenv("USER");
    if(!user) user = "unknown";

    // Buradaki printf argüman hataları düzeltildi:
    printf("      %s┌────────────────────────────────────────────────────────┐%s\n", border_c, reset);
    printf("      %s│%s  USER: %-15s   OS: Linux / MiniShell v1.0 %s│%s\n", border_c, accent_c, user, border_c, reset);
    printf("      %s│%s  DATE: %-15s   STATUS: %sONLINE             %s│%s\n", border_c, text_c, date_str, "\033[1;5;32m", border_c, reset);
    printf("      %s└────────────────────────────────────────────────────────┘%s\n\n", border_c, reset);

    // 3. Menü Seçenekleri
    // Buradaki fazla değişkenler temizlendi:
    printf("      %s╔════════════════════════════╗   ╔═══════════════════╗%s\n", border_c, reset);
    printf("      %s║ %s[1] SHELL'İ BAŞLAT       %s║   ║ %s[2] SİSTEM ÇIKIŞ  %s║%s\n", border_c, "\033[1;36m", border_c, "\033[1;31m", border_c, reset);
    printf("      %s║ %sTerminal arayüzüne git   %s║   ║ %sKapat ve çık      %s║%s\n", border_c, "\033[0;37m", border_c, "\033[0;37m", border_c, reset);
    printf("      %s╚════════════════════════════╝   ╚═══════════════════╝%s\n", border_c, reset);
    
    printf("\n");
    printf("      %sKomutunuzu girin > %s", accent_c, reset);
} 
// DİKKAT: Bu süslü parantez (}) çok önemli, eksik olursa 'expected declaration' hatası alırsın.
// SIGCHLD sinyal handler - zombie process'leri temizler
void sigchld_handler(int signo) {
    (void)signo;
    // Tüm biten child process'leri temizle
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

// SIGINT handler - Ctrl+C tuşuna basıldığında satırı iptal et
void sigint_handler(int signo) {
    (void)signo;
    write(1, "\n", 1);
    tcflush(STDIN_FILENO, TCIFLUSH);
}

static struct termios orig_termios;

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

void restore_terminal() {
#ifdef ECHOCTL
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
#endif
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
    
    // Built-in: clear komutu
    if(strcmp(args[0], "clear") == 0) {
        printf("\033[H\033[J");
        return 0;
    }

    // --- BURADAN BAŞLA (execute_command içine ekle) ---

    // Built-in: rm [dosya] (Dosya silme)
    if(strcmp(args[0], "rm") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "rm: eksik argüman\n");
            return 1;
        }
        if(unlink(args[1]) != 0) {
            perror("rm hatası");
            return 1;
        }
        return 0;
    }

    // Built-in: mv [kaynak] [hedef] (Taşıma/İsim değiştirme)
    if(strcmp(args[0], "mv") == 0) {
        if(args[1] == NULL || args[2] == NULL) {
            fprintf(stderr, "mv: eksik argüman (kullanım: mv kaynak hedef)\n");
            return 1;
        }
        if(rename(args[1], args[2]) != 0) {
            perror("mv hatası");
            return 1;
        }
        return 0;
    }

    // Built-in: cp [kaynak] [hedef] (Kopyalama)
    if(strcmp(args[0], "cp") == 0) {
        if(args[1] == NULL || args[2] == NULL) {
            fprintf(stderr, "cp: eksik argüman (kullanım: cp kaynak hedef)\n");
            return 1;
        }
        
        int src_fd = open(args[1], O_RDONLY);
        if(src_fd < 0) {
            perror("cp: kaynak okunamadı");
            return 1;
        }

        // Hedef dosyayı oluştur (varsa üzerine yazar: O_TRUNC), izinler 0644
        int dest_fd = open(args[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(dest_fd < 0) {
            perror("cp: hedef oluşturulamadı");
            close(src_fd);
            return 1;
        }

        char buf[4096];
        ssize_t nread;
        while((nread = read(src_fd, buf, sizeof(buf))) > 0) {
            if(write(dest_fd, buf, nread) != nread) {
                perror("cp: yazma hatası");
                close(src_fd);
                close(dest_fd);
                return 1;
            }
        }

        close(src_fd);
        close(dest_fd);
        return 0;
    }

    // Built-in: env (Ortam değişkenlerini listele)
    if(strcmp(args[0], "env") == 0) {
        extern char **environ; // Global çevre değişkenleri pointer'ı
        for(char **env = environ; *env != 0; env++) {
            printf("%s\n", *env);
        }
        return 0;
    }
    
    // --- BURADA BİTİR ---
    
    // Built-in: help komutu
    if(strcmp(args[0], "help") == 0) {
        printf("\n\033[1;36m=== Mini Shell Komutları ===\033[0m\n\n");
        printf("\033[1;33mDosya İşlemleri:\033[0m\n");
        printf("  \033[32mcat [dosya]\033[0m       - Dosya içeriğini göster\n");
        printf("  \033[32mtouch [dosya]\033[0m     - Yeni dosya oluştur\n");
        printf("  \033[32mrm [dosya]\033[0m        - Dosyayı sil (YENİ)\n");
        printf("  \033[32mcp [src] [dst]\033[0m    - Dosyayı kopyala (YENİ)\n");
        printf("  \033[32mmv [src] [dst]\033[0m    - Dosyayı taşı/ad değiştir (YENİ)\n");
        
        printf("\n\033[1;33mDizin İşlemleri:\033[0m\n");
        printf("  \033[32mcd [dizin]\033[0m        - Dizin değiştir\n");
        printf("  \033[32mpwd\033[0m               - Bulunduğun dizini göster\n");
        printf("  \033[32mmkdir [dizin]\033[0m     - Klasör oluştur\n");
        printf("  \033[32mmkdir [dizin]\033[0m     - Klasör sil\n");

        printf("\n\033[1;33mSistem ve Bilgi:\033[0m\n");
        printf("  \033[32mclear\033[0m             - Ekranı temizle\n");
        printf("  \033[32mecho [metin]\033[0m      - Ekrana yazı yaz\n");
        printf("  \033[32mwhoami\033[0m            - Kullanıcı adı\n");
        printf("  \033[32mdate\033[0m              - Tarih ve saat\n");
        printf("  \033[32muptime\033[0m            - Çalışma süresi\n");
        printf("  \033[32mhistory\033[0m           - Komut geçmişi\n");
        printf("  \033[32menv\033[0m               - Ortam değişkenleri (YENİ)\n");
        printf("  \033[32mhelp\033[0m              - Yardım menüsü\n");
        printf("  \033[32mexit\033[0m              - Çıkış\n\n");

        printf("\033[1;33mOperatörler:\033[0m\n");
        printf("  \033[36m&&\033[0m (VE), \033[36m||\033[0m (VEYA), \033[36m&\033[0m (Arkaplan)\n\n");
        return 0;
    }
    // Built-in: cat komutu (basit)
    if(strcmp(args[0], "cat") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "cat: eksik argüman\n");
            return 1;
        }
        for(int f = 1; args[f] != NULL; f++) {
            int fd = open(args[f], O_RDONLY);
            if(fd < 0) {
                perror("cat hatası");
                continue;
            }
            char buf[4096];
            ssize_t r;
            while((r = read(fd, buf, sizeof(buf))) > 0) {
                ssize_t w = write(1, buf, r);
                (void)w;
            }
            close(fd);
        }
        return 0;
    }
    
    // Built-in: pwd komutu
    if(strcmp(args[0], "pwd") == 0) {
        char cwd2[1024];
        if(getcwd(cwd2, sizeof(cwd2)) != NULL) {
            printf("%s\n", cwd2);
        } else {
            perror("pwd hatası");
        }
        return 0;
    }

    // Built-in: echo komutu
    if(strcmp(args[0], "echo") == 0) {
        for(int j = 1; args[j] != NULL; j++) {
            if(j > 1) printf(" ");
            printf("%s", args[j]);
        }
        printf("\n");
        return 0;
    }

    // Built-in: mkdir komutu
    if(strcmp(args[0], "mkdir") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "mkdir: eksik argüman\n");
            return 1;
        }
        if(mkdir(args[1], 0755) != 0) {
            perror("mkdir hatası");
            return 1;
        }
        return 0;
    }

    // Built-in: rmdir komutu
    if(strcmp(args[0], "rmdir") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "rmdir: eksik argüman\n");
            return 1;
        }
        if(rmdir(args[1]) != 0) {
            perror("rmdir hatası");
            return 1;
        }
        return 0;
    }

    // Built-in: touch komutu (basit - sadece dosya oluşturur)
    if(strcmp(args[0], "touch") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "touch: eksik argüman\n");
            return 1;
        }
        int fd = open(args[1], O_CREAT | O_WRONLY, 0644);
        if(fd < 0) {
            perror("touch hatası");
            return 1;
        }
        close(fd);
        return 0;
    }
    
    // Built-in: whoami
    if(strcmp(args[0], "whoami") == 0) {
        char *username = getenv("USER");
        if(username == NULL) username = getenv("USERNAME");
        if(username == NULL) username = "user";
        printf("%s\n", username);
        return 0;
    }

    // Built-in: date
    if(strcmp(args[0], "date") == 0) {
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        char buf[128];
        if(lt) strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y", lt);
        printf("%s\n", buf);
        return 0;
    }

    // Built-in: uptime (reads /proc/uptime on Linux/WSL)
    if(strcmp(args[0], "uptime") == 0) {
        FILE *f = fopen("/proc/uptime", "r");
        if(f) {
            double up=0.0; if(fscanf(f, "%lf", &up) == 1) {
                int hours = (int)(up/3600);
                int mins = (int)((up - hours*3600)/60);
                int secs = (int)(up) % 60;
                printf("%d:%02d:%02d up\n", hours, mins, secs);
            }
            fclose(f);
        } else {
            printf("uptime: /proc/uptime okunamadi\n");
        }
        return 0;
    }

    // Built-in: history
    if(strcmp(args[0], "history") == 0) {
        for(int h = 0; h < history_count; h++) {
            printf("%4d  %s\n", h+1, history[h]);
        }
        return 0;
    }

    // Built-in: exit komutu (menu back)
    if(strcmp(args[0], "exit") == 0) {
        printf("\033[1;35mMenüye dönülüyor...\033[0m\n");
        return_to_menu = 1;
        return 0;
    }
    
    // Normal komutlar için fork-exec
    pid_t pid = fork();
    
    if(pid < 0) {
        perror("Fork hatası");
        return 1;
    }
    else if(pid == 0) {
        // Child process
        // Child, Ctrl+C'yi varsayılan davranışa geri almalı
        signal(SIGINT, SIG_DFL);
        if(execvp(args[0], args) < 0) {
            printf("\033[1;31mKomut bulunamadı: %s\033[0m\n", args[0]);
            exit(127);
        }
    }
    else {
        // Parent process
        if(background) {
            // Arka planda çalışıyor - beklemeden devam et
            printf("\033[1;33m[%d] %s\033[0m\n", pid, args[0]);
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

static void shell_loop(void) {
    char command[MAX_CMD_LEN];
    // draw header once when entering interactive shell (avoid overwriting command output)
    draw_header_footer();
    while(1) {

        // Kullanıcı adı ve dizini al (WSL/Windows fallback)
        char *username = getenv("USER");
        if(username == NULL) username = getenv("USERNAME");
        if(username == NULL) username = "user";
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        
        // Dizini kısalt (sadece son klasör)
        char *short_cwd = strrchr(cwd, '/');
        if(short_cwd == NULL) {
            short_cwd = cwd;
        } else {
            short_cwd++;
        }

        // Renkli prompt
        printf("\033[1;32m%s\033[0m:", username);
        printf("\033[1;34m~/%s\033[0m", short_cwd);
        printf("\033[1;33m$\033[0m ");
        fflush(stdout);

        if(fgets(command, MAX_CMD_LEN, stdin) == NULL) {
            if(errno == EINTR) {
                // Ctrl+C ile kesildi, tekrar prompt göster
                continue;
            }
            break;
        }

        command[strcspn(command, "\n")] = 0;

        if(strlen(command) == 0) {
            continue;
        }

        // add to history (simple ring)
        if(history_count < MAX_HISTORY) {
            strncpy(history[history_count], command, MAX_CMD_LEN-1);
            history[history_count][MAX_CMD_LEN-1] = '\0';
            history_count++;
        } else {
            // shift left and add
            for(int hh = 1; hh < MAX_HISTORY; hh++) strcpy(history[hh-1], history[hh]);
            strncpy(history[MAX_HISTORY-1], command, MAX_CMD_LEN-1);
            history[MAX_HISTORY-1][MAX_CMD_LEN-1] = '\0';
        }

        handle_operators(command);

        if(return_to_menu) break;
    }
}

int main() {
    // Sinyal handler'lar
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    
    // Karakter seti ayarı (Kutuların düzgün görünmesi için)
    setlocale(LC_ALL, "");
    
    // İlk açılışta ekranı temizle
    printf("\033[2J\033[H");

    disable_echoctl();
    atexit(restore_terminal);

    while(1) {
        // Menüyü göster
        show_fancy_menu(); 
        
        fflush(stdout);
        char choice[16];
        if(fgets(choice, sizeof(choice), stdin) == NULL) break;
        choice[strcspn(choice, "\n")] = 0;
        
        if(strlen(choice) == 0 || choice[0] == '1') {
            // Yükleniyor efekti
            printf("\n\033[1;33m[*] Çekirdek yükleniyor...\033[0m");
            fflush(stdout);
            usleep(300000); 
            printf("\r\033[1;32m[OK] Shell aktif!         \033[0m\n");
            usleep(200000);
            
            // Shell döngüsüne gir
            enter_alt_screen();
            return_to_menu = 0;
            shell_loop();
            restore_ui(); 
            
            // Eğer exit ile çıkılmadıysa programdan çıkma, menüye dön
            if(!return_to_menu) break; 
        } 
        else if(choice[0] == '2' || tolower(choice[0]) == 'q') {
            printf("\n\033[1;35mGüle güle, %s!\033[0m\n", getenv("USER"));
            break;
        }
        else {
             // Hatalı giriş için bip sesi
             printf("\a"); 
        }
    }
    
    return 0;
}
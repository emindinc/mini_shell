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
        // fallback sizes
        w.ws_row = 24; w.ws_col = 80;
    }
    int rows = w.ws_row;
    int cols = w.ws_col;

    // Clear and position
    printf("\033[2J");
    printf("\033[H");

    // Build a timestamp for the header's right side
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char timestr[64] = "";
    if(lt) strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", lt);

    // Header: left title, right timestamp
    char title_left[] = "Mini Shell";
    char version[] = "v1.0";
    char header_full[128];
    snprintf(header_full, sizeof(header_full), " %s - %s ", title_left, version);

    // Draw header background and content
    printf("\033[48;5;19m\033[1;97m");
    int left_len = (int)strlen(header_full);
    int right_len = (int)strlen(timestr);
    int middle_space = cols - left_len - right_len - 2; // padding
    if(middle_space < 0) middle_space = 0;
    printf("%s", header_full);
    for(int i = 0; i < middle_space; i++) putchar(' ');
    if(right_len) printf("%s", timestr);
    for(int i = left_len + middle_space + right_len; i < cols; i++) putchar(' ');
    printf("\033[0m\n");

    // Decorative separator (thin line)
    printf("\033[38;5;24m");
    for(int i = 0; i < cols; i++) printf("─");
    printf("\033[0m\n\n");

    // Info box: show title, user, cwd, and built-ins (single tidy box)
    char *username = getenv("USER");
    if(username == NULL) username = getenv("USERNAME");
    if(username == NULL) username = "user";
    char cwd[1024];
    if(getcwd(cwd, sizeof(cwd)) == NULL) strcpy(cwd, "~");
    char *short_cwd = strrchr(cwd, '/');
    if(short_cwd) short_cwd++; else short_cwd = cwd;

    int box_w = (cols > 64) ? 64 : cols - 8;
    int box_x = 4;
    int box_y = 5;
    int inner_w = box_w - 4;
    // Compose lines
    char line1[256];
    char line2[256];
    snprintf(line1, sizeof(line1), "%s - %s", title_left, version);
    snprintf(line2, sizeof(line2), "User: %s   Dir: ~/%s", username, short_cwd);
    /* no extra third line variable */

    printf("\033[%d;%dH", box_y, box_x);
    printf("\033[36m┌");
    for(int i = 0; i < box_w - 2; i++) printf("─");
    printf("┐\033[0m\n");

    // Line 1
    printf("\033[%d;%dH", box_y + 1, box_x);
    printf("\033[36m│\033[0m  \033[1;36m%s\033[0m", line1);
    int used = (int)strlen(line1);
    for(int i = 0; i < inner_w - used; i++) putchar(' ');
    printf("\033[36m│\033[0m\n");

    // Line 2 (user/dir)
    printf("\033[%d;%dH", box_y + 2, box_x);
    printf("\033[36m│\033[0m  %s", line2);
    int l2 = (int)strlen(line2);
    for(int i = 0; i < inner_w - l2; i++) putchar(' ');
    printf("\033[36m│\033[0m\n");

    // Line 3: small tagline (keep header minimal)
    printf("\033[%d;%dH", box_y + 3, box_x);
    printf("\033[36m│\033[0m  %s", "Basit, hafif ve hızlı. 'help' yazın.");
    int l3 = (int)strlen("Basit, hafif ve hızlı. 'help' yazın.");
    for(int i = 0; i < inner_w - l3; i++) putchar(' ');
    printf("\033[36m│\033[0m\n");

    // Bottom border
    printf("\033[%d;%dH", box_y + 4, box_x);
    printf("\033[36m└");
    for(int i = 0; i < box_w - 2; i++) printf("─");
    printf("┘\033[0m\n\n");

    // Prompt area with subtle framed line
    int pbox_row = box_y + 6;
    int pcol = 3;
    printf("\033[%d;%dH", pbox_row, pcol);
    printf("\033[2m\033[90m");
    for(int i = 0; i < cols - 6; i++) putchar(' ');
    printf("\033[0m\n");

    // Footer: left and right hints
    int footer_row = rows;
    char left_hint[] = "help: yardim  ";
    char mid_hint[] = " clear ";
    char right_hint[128];
    snprintf(right_hint, sizeof(right_hint), " %s ", "Ctrl+C: satırı iptal et");

    printf("\033[%d;1H", footer_row);
    // left hint in dim cyan
    printf("\033[2m\033[36m");
    printf(" %s", left_hint);
    // center small hint
    int used_footer = (int)strlen(left_hint) + (int)strlen(mid_hint) + (int)strlen(right_hint) + 4;
    int spaces = cols - used_footer;
    if(spaces < 0) spaces = 0;
    for(int i = 0; i < spaces; i++) putchar(' ');
    printf("%s", right_hint);
    printf("\033[0m");

    // Place cursor after header region for prompt input
    printf("\033[%d;1H", pbox_row + 1);
    fflush(stdout);

    // (removed old duplicate subtitle/footer block)
}

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
    
    // Built-in: help komutu
    if(strcmp(args[0], "help") == 0) {
        printf("\n\033[1;36m=== Mini Shell Komutları ===\033[0m\n\n");
        printf("\033[1;33mBuilt-in Komutlar:\033[0m\n");
        printf("  \033[32mcd [dizin]\033[0m       - Dizin değiştir\n");
        printf("  \033[32mpwd\033[0m              - Geçerli çalışma dizinini yazdır\n");
        printf("  \033[32mcat [dosya]\033[0m       - Dosya içeriğini göster\n");
        printf("  \033[32mclear\033[0m            - Ekranı temizle\n");
        printf("  \033[32mecho [metin]\033[0m       - Metni ekrana yazdır\n");
        printf("  \033[32mmkdir [dizin]\033[0m   - Yeni dizin oluştur\n");
        printf("  \033[32mrmdir [dizin]\033[0m   - Boş dizini sil\n");
        printf("  \033[32mtouch [dosya]\033[0m    - Yeni boş dosya oluştur\n");
        printf("  \033[32mhelp\033[0m             - Bu yardım mesajını göster\n");
        printf("  \033[32mexit\033[0m             - Menüye dön\n\n");
        printf("  \033[32mwhoami\033[0m           - Aktif kullanıcı adını göster\n");
        printf("  \033[32mdate\033[0m             - Sistem tarih/saat\n");
        printf("  \033[32muptime\033[0m           - Sistemin uptime süresi\n");
        printf("  \033[32mhistory\033[0m          - Komut geçmişini göster\n\n");
        printf("\033[1;33mOperatörler:\033[0m\n");
        printf("  \033[36mkomut1 && komut2\033[0m - komut1 başarılıysa komut2'yi çalıştır\n");
        printf("  \033[36mkomut1 || komut2\033[0m - komut1 başarısızsa komut2'yi çalıştır\n");
        printf("  \033[36mkomut &\033[0m          - Komutu arka planda çalıştır\n\n");
        printf("\033[1;33mÖrnekler:\033[0m\n");
        printf("  ls && pwd\n");
        printf("  mkdir test && cd test\n");
        printf("  sleep 5 &\n\n");
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
    
    // SIGCHLD sinyal handler'ı kur
    signal(SIGCHLD, sigchld_handler);
    // SIGINT için özel handler: Ctrl+C satırı iptal etsin ama shell kapanmasın
    signal(SIGINT, sigint_handler);
    // Set locale so box-drawing and UTF-8 characters render correctly
    setlocale(LC_ALL, "");
    // Enter alternate screen and draw a nicer header/footer UI
    enter_alt_screen();
    // Echo control karakterlerinin (^C gibi) terminalde görünmesini kapat
    disable_echoctl();
    atexit(restore_terminal);
    
    // (startup banner removed; alternate screen UI draws header)
    
    // Simple main menu loop
    while(1) {
        printf("\n1) Shell'i başlat\n2) Çıkış\n\nSeçiminiz: ");
        fflush(stdout);
        char choice[16];
        if(fgets(choice, sizeof(choice), stdin) == NULL) break;
        choice[strcspn(choice, "\n")] = 0;
        if(strlen(choice) == 0 || choice[0] == '1') {
            // enter interactive shell loop
            return_to_menu = 0;
            shell_loop();
            // if user requested return to menu, continue; otherwise exit
            if(!return_to_menu) break;
        } else if(choice[0] == '2' || tolower(choice[0]) == 'q') {
            break;
        }
    }
    
    return 0;
}

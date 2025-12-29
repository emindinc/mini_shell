#include <stdio.h>
#include <sys/sysinfo.h> // Sistem bilgisi için (RAM vs)
#include <sys/utsname.h> // Kernel bilgisi için
#include <math.h>        // Hesap makinesi için
#include <dirent.h>
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

// --- GLOBAL DEĞİŞKENLER ---
const char *tips[] = {
    "İpucu: 'help' yazarak tüm komutları görebilirsiniz.",
    "İpucu: 'calc' ile matematik işlemleri yapabilirsiniz.",
    "İpucu: Çıkmak için menüde '2'ye basın veya shell'de 'exit' yazın.",
    "İpucu: 'history' ile geçmiş komutlarınıza ulaşın.",
    "İpucu: Dosya izinleri için 'chmod' kullanabilirsiniz.",
    "Bilgi: Bu shell C dili ile geliştirilmiştir.",
    "Sistem: RAM durumunu 'free' komutuyla kontrol edin."
};

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
// Dashboard Tarzı Menü
// Dashboard Tarzı Menü (DÜZELTİLMİŞ)
void show_fancy_menu() {
    // Ekranı temizle
    printf("\033[2J\033[H");
    
    // Renkler
    char *c_border = "\033[38;5;93m";  // Mor
    char *c_title  = "\033[1;36m";     // Cyan
    char *c_text   = "\033[1;97m";     // Beyaz
    char *c_green  = "\033[1;32m";     // Yeşil
    char *c_warn   = "\033[1;33m";     // Sarı
    char *c_reset  = "\033[0m";

    // Sistem Bilgisi Al
    struct sysinfo si;
    long ram_total = 0, ram_used = 0;
    if(sysinfo(&si) == 0) {
        ram_total = si.totalram / 1024 / 1024;
        ram_used = (si.totalram - si.freeram) / 1024 / 1024;
    }

    // Rastgele İpucu Seç
    int tip_index = rand() % 7; 

    printf("\n");
    // Logo
    printf("      %s    __  __ _       _   ____  _          _ _ %s\n", c_title, c_reset);
    printf("      %s   |  \\/  (_)_ __ (_) / ___|| |__   ___| | |%s\n", c_title, c_reset);
    printf("      %s   | |\\/| | | '_ \\| | \\___ \\| '_ \\ / _ \\ | |%s\n", c_title, c_reset);
    printf("      %s   |_|  |_|_|_| |_|_| |____/|_| |_|\\___|_|_|%s\n", c_title, c_reset);
    printf("\n");

    // Üst Çerçeve
    printf("      %s╔════════════════════ SYSTEM STATUS ════════════════════╗%s\n", c_border, c_reset);
    
    // Sistem Durumu Satırı (RAM Gösterimi Güncellendi: Used/Total)
    printf("      %s║%s   HOST: %-12s  CPU: [OK]    MEM: %4ld/%ld MB %s║%s\n", 
           c_border, c_text, getenv("USER"), ram_used, ram_total, c_border, c_reset);
           
    // Tarih Satırı
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char d_str[32];
    strftime(d_str, sizeof(d_str), "%H:%M:%S", &tm);
    printf("      %s║%s   TIME: %-12s  NET: %sONLINE  %sBAT: %s%%100    %s║%s\n", 
           c_border, c_text, d_str, c_green, c_text, c_green, c_border, c_reset);

    printf("      %s╚═══════════════════════════════════════════════════════╝%s\n", c_border, c_reset);
    printf("\n");

    // Butonlar (Printf Hatası Giderildi)
    // Düzeltme: Değişken sayısı ve %s sayısı eşitlendi.
    printf("      %s┌──────────────────────┐      ┌──────────────────────┐%s\n", c_border, c_reset);
    printf("      %s│ %s[1] SHELL BAŞLAT     %s│      │ %s[2] GÜVENLİ ÇIKIŞ  %s│%s\n", 
           c_border, c_green, c_border, c_warn, c_border, c_reset); 
    printf("      %s└──────────────────────┘      └──────────────────────┘%s\n", c_border, c_reset);

    // Alt Bilgi / İpucu Alanı
    printf("\n");
    printf("      %s> %s%s%s\n", c_border, "\033[3m", tips[tip_index], c_reset);
    printf("\n");
    printf("      %sSEÇİMİNİZ > %s", c_title, c_reset);
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
    // Alternate Screen modundan çık (Eski terminale tertemiz dön)
    printf("\033[?1049l"); 
    
    // İmleci görünür yap
    printf("\033[?25h"); 
    
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

    // --- YENİ KOMUTLAR BAŞLANGIÇ ---

    // Built-in: ls (Renkli Listeleme)
    if(strcmp(args[0], "ls") == 0) {
        DIR *d;
        struct dirent *dir;
        d = opendir("."); // Şu anki klasörü aç
        if (d) {
            int count = 0;
            while ((dir = readdir(d)) != NULL) {
                // Gizli dosyaları (. ile başlayan) atla (isteğe bağlı)
                if(dir->d_name[0] == '.') continue;

                // Klasör mü dosya mı? (Basit renk ayrımı)
                // Not: DT_DIR her dosya sisteminde desteklenmeyebilir ama genelde çalışır.
                if(dir->d_type == DT_DIR) {
                    printf("\033[1;34m%s/\033[0m  ", dir->d_name); // Klasörler Mavi
                } else {
                    printf("\033[0m%s  ", dir->d_name); // Dosyalar Beyaz
                }
                
                count++;
                if(count % 5 == 0) printf("\n"); // 5 dosyada bir alt satıra geç
            }
            printf("\n");
            closedir(d);
        } else {
            perror("ls hatası");
            return 1;
        }
        return 0;
    }

    // Built-in: grep [kelime] [dosya] (Basit arama)
    if(strcmp(args[0], "grep") == 0) {
        if(args[1] == NULL || args[2] == NULL) {
            fprintf(stderr, "Kullanım: grep [aranan] [dosya]\n");
            return 1;
        }
        FILE *fp = fopen(args[2], "r");
        if(!fp) { perror("grep dosya hatası"); return 1; }
        
        char line[1024];
        int line_num = 1;
        while(fgets(line, sizeof(line), fp)) {
            // Aranan kelime satırda var mı?
            if(strstr(line, args[1]) != NULL) {
                // Satır numarasını mor, satırı normal bas
                printf("\033[1;35m%d:\033[0m %s", line_num, line);
                // Eğer satır sonunda \n yoksa ekle
                if(line[strlen(line)-1] != '\n') printf("\n");
            }
            line_num++;
        }
        fclose(fp);
        return 0;
    }

    // Built-in: wc [dosya] (Satır sayma - Word Count)
    if(strcmp(args[0], "wc") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "Kullanım: wc [dosya]\n");
            return 1;
        }
        FILE *fp = fopen(args[1], "r");
        if(!fp) { perror("wc hata"); return 1; }
        
        int lines = 0;
        char ch;
        while(!feof(fp)) {
            ch = fgetc(fp);
            if(ch == '\n') lines++;
        }
        fclose(fp);
        printf("\033[1;32m%d\033[0m satır\n", lines);
        return 0;
    }

    // Built-in: head [dosya] (İlk 10 satır)
    if(strcmp(args[0], "head") == 0) {
        if(args[1] == NULL) {
            fprintf(stderr, "Kullanım: head [dosya]\n");
            return 1;
        }
        FILE *fp = fopen(args[1], "r");
        if(!fp) { perror("head hata"); return 1; }
        
        char line[1024];
        int count = 0;
        while(fgets(line, sizeof(line), fp) && count < 10) {
            printf("%s", line);
            count++;
        }
        fclose(fp);
        return 0;
    }

    // --- YENİ KOMUTLAR BİTİŞ ---
    
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
    // Built-in: help komutu (GÜNCELLENMİŞ - 25+ KOMUT)
    if(strcmp(args[0], "help") == 0) {
        printf("\n\033[1;36m=== Mini Shell Komut Merkezi (%s) ===\033[0m\n\n", "v1.1");
        
        char *c_cmd = "\033[1;32m"; // Yeşil
        char *c_cmt = "\033[0;90m"; // Gri
        char *rst = "\033[0m";      // Reset

        printf("  %sDosya ve Dizin:%s\n", "\033[1;33m", rst);
        printf("  %sls%s             %s# Dosyaları renkli listele\n", c_cmd, rst, c_cmt);
        printf("  %scd [yol]%s       %s# Dizin değiştir\n", c_cmd, rst, c_cmt);
        printf("  %spwd%s            %s# Çalışma dizinini göster\n", c_cmd, rst, c_cmt);
        printf("  %smkdir [ad]%s     %s# Klasör oluştur\n", c_cmd, rst, c_cmt);
        printf("  %srmdir [ad]%s     %s# Klasör sil\n", c_cmd, rst, c_cmt);
        printf("  %stouch [ad]%s     %s# Dosya oluştur\n", c_cmd, rst, c_cmt);
        printf("  %srm [ad]%s        %s# Dosya sil\n", c_cmd, rst, c_cmt);
        printf("  %scp [k] [h]%s     %s# Dosya kopyala\n", c_cmd, rst, c_cmt);
        printf("  %smv [k] [h]%s     %s# Dosya taşı/isim değiştir\n", c_cmd, rst, c_cmt);
        printf("  %schmod [m] [f]%s  %s# Dosya izni değiştir (chmod 777 f) (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %sstat [f]%s       %s# Dosya detaylarını gör (YENİ)\n", c_cmd, rst, c_cmt);

        printf("\n  %sİçerik ve Arama:%s\n", "\033[1;33m", rst);
        printf("  %scat [f]%s        %s# Dosyayı oku\n", c_cmd, rst, c_cmt);
        printf("  %shead [f]%s       %s# Dosyanın başını oku (ilk 10)\n", c_cmd, rst, c_cmt);
        printf("  %stail [f]%s       %s# Dosyanın sonunu oku (son 10) (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %sgrep [k] [f]%s   %s# Dosyada kelime ara\n", c_cmd, rst, c_cmt);
        printf("  %swc [f]%s         %s# Satır sayısı say\n", c_cmd, rst, c_cmt);

        printf("\n  %sSistem ve Araçlar:%s\n", "\033[1;33m", rst);
        printf("  %scalc [işlem]%s   %s# Hesap makinesi (calc 5 + 3) (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %sfree%s           %s# RAM kullanımını göster (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %sdf%s             %s# Disk kullanımını göster (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %sps%s             %s# Çalışan işlemleri gör (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %skill [pid]%s     %s# İşlemi sonlandır (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %shostname%s       %s# Bilgisayar adını gör (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %srand%s           %s# Rastgele sayı üret (YENİ)\n", c_cmd, rst, c_cmt);
        printf("  %sdate%s           %s# Tarih ve saat\n", c_cmd, rst, c_cmt);
        printf("  %swhoami%s         %s# Kullanıcı adı\n", c_cmd, rst, c_cmt);
        printf("  %shistory%s        %s# Komut geçmişi\n", c_cmd, rst, c_cmt);
        printf("  %sclear%s          %s# Temizle\n", c_cmd, rst, c_cmt);
        printf("  %sexit%s           %s# Çıkış\n\n", c_cmd, rst, c_cmt);
        
        printf("%s", rst); // Renk sıfırlama
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
    // --- YENİ EKLENEN 10 KOMUT ---

    // 1. Built-in: tail [dosya] (Son 10 satırı göster)
    if(strcmp(args[0], "tail") == 0) {
        if(args[1] == NULL) { fprintf(stderr, "Kullanım: tail [dosya]\n"); return 1; }
        FILE *fp = fopen(args[1], "r");
        if(!fp) { perror("tail hata"); return 1; }
        // Satır sayısını bul
        int total_lines = 0;
        char ch;
        while(!feof(fp)) { if(fgetc(fp) == '\n') total_lines++; }
        rewind(fp); // Başa dön
        
        int current_line = 0;
        char line[1024];
        while(fgets(line, sizeof(line), fp)) {
            if(current_line >= total_lines - 10) printf("%s", line);
            current_line++;
        }
        fclose(fp);
        return 0;
    }

    // 2. Built-in: chmod [mod] [dosya] (İzin değiştirme örn: chmod 777 dosya)
    if(strcmp(args[0], "chmod") == 0) {
        if(args[1] == NULL || args[2] == NULL) {
            fprintf(stderr, "Kullanım: chmod [mod] [dosya] (örn: 755)\n");
            return 1;
        }
        int mod = strtol(args[1], NULL, 8); // Octal (8'lik) tabana çevir
        if(chmod(args[2], mod) < 0) { perror("chmod hatası"); return 1; }
        return 0;
    }

    // 3. Built-in: stat [dosya] (Dosya detayları)
    if(strcmp(args[0], "stat") == 0) {
        if(args[1] == NULL) { fprintf(stderr, "Kullanım: stat [dosya]\n"); return 1; }
        struct stat st;
        if(stat(args[1], &st) == 0) {
            printf("Dosya: %s\n", args[1]);
            printf("Boyut: %ld byte\n", st.st_size);
            printf("İzinler: %o\n", st.st_mode & 0777);
            printf("User ID: %d\n", st.st_uid);
        } else { perror("stat hatası"); return 1; }
        return 0;
    }

    // 4. Built-in: calc [sayı1] [işlem] [sayı2] (Basit hesap makinesi)
    if(strcmp(args[0], "calc") == 0) {
        if(args[1] == NULL || args[2] == NULL || args[3] == NULL) {
            printf("Kullanım: calc 10 + 5\nDesteklenen: + - * / %%\n");
            return 1;
        }
        double n1 = atof(args[1]);
        double n2 = atof(args[3]);
        char op = args[2][0];
        double res = 0;
        
        switch(op) {
            case '+': res = n1 + n2; break;
            case '-': res = n1 - n2; break;
            case '*': res = n1 * n2; break;
            case '/': if(n2!=0) res = n1 / n2; else {printf("Hata: 0'a bölünmez!\n"); return 1;} break;
            default: printf("Geçersiz işlem!\n"); return 1;
        }
        printf("Sonuç: %.2f\n", res);
        return 0;
    }

    // 5. Built-in: rand (Rastgele sayı üret)
    if(strcmp(args[0], "rand") == 0) {
        srand(time(NULL));
        printf("%d\n", rand() % 1000); // 0-999 arası
        return 0;
    }

    // 6. Built-in: hostname (Bilgisayar adı)
    if(strcmp(args[0], "hostname") == 0) {
        char host[256];
        if(gethostname(host, sizeof(host)) == 0) printf("%s\n", host);
        else perror("hostname hatası");
        return 0;
    }

    // 7. Built-in: free (RAM kullanımı - Linux/WSL)
    if(strcmp(args[0], "free") == 0) {
        struct sysinfo si;
        if(sysinfo(&si) == 0) {
            printf("Toplam RAM: %lu MB\n", si.totalram / 1024 / 1024);
            printf("Boş RAM   : %lu MB\n", si.freeram / 1024 / 1024);
            printf("Kullanılan: %lu MB\n", (si.totalram - si.freeram) / 1024 / 1024);
        } else perror("free hatası");
        return 0;
    }

    // 8. Built-in: df (Basit Disk bilgisi)
    if(strcmp(args[0], "df") == 0) {
        // Sistem çağrısı yerine basitçe df -h çalıştırıp çıktıyı alalım (daha pratik)
        system("df -h ."); 
        return 0;
    }

    // 9. Built-in: kill [pid] (Process öldür)
    if(strcmp(args[0], "kill") == 0) {
        if(args[1] == NULL) { fprintf(stderr, "Kullanım: kill [PID]\n"); return 1; }
        pid_t pid = atoi(args[1]);
        if(kill(pid, SIGKILL) == 0) printf("Process %d sonlandırıldı.\n", pid);
        else perror("kill hatası");
        return 0;
    }

    // 10. Built-in: ps (Çalışan işlemleri listele - basit)
    if(strcmp(args[0], "ps") == 0) {
        printf("PID\tCMD\n");
        // /proc klasörünü okumak karmaşık olduğu için sistem ps'ini çağıralım
        system("ps -e | head -n 10"); // Sadece ilk 10 tanesi
        return 0;
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
    // Rastgele sayı üreteci (ipuçları için)
    srand(time(NULL));
    
    // Sinyal handler'lar
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    
    setlocale(LC_ALL, "");

    // --- BURASI ÇOK ÖNEMLİ: ALTERNATE SCREEN MODU AÇILIYOR ---
    // Bu kod terminali "Tam Ekran Uygulama" moduna sokar (vim gibi).
    // Scroll bar kaybolur, eski çıktılar görünmez.
    printf("\033[?1049h");
    fflush(stdout);
    // ---------------------------------------------------------

    disable_echoctl();
    // Program kapanırken restore_terminal çalışacak ve eski ekrana dönecek
    atexit(restore_terminal);

    while(1) {
        show_fancy_menu(); 
        
        fflush(stdout);
        char choice[16];
        if(fgets(choice, sizeof(choice), stdin) == NULL) break;
        choice[strcspn(choice, "\n")] = 0;
        
        if(strlen(choice) == 0 || choice[0] == '1') {
            printf("\n\033[1;32m[*] Shell başlatılıyor...\033[0m");
            fflush(stdout);
            usleep(200000); 
            
            // Shell döngüsüne gir
            // enter_alt_screen() çağrısını sildik çünkü zaten main başında girdik.
            return_to_menu = 0;
            shell_loop();
            
            // Eğer exit ile çıkılmadıysa döngü devam eder
            if(!return_to_menu) break; 
        } 
        else if(choice[0] == '2' || tolower(choice[0]) == 'q') {
            break;
        }
    }
    
    return 0;
}
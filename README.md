# Mini Shell

İşletim Sistemleri dersi için geliştirilmiş, gelişmiş özelliklere sahip komut yorumlayıcısı.

## 🎯 Özellikler

### ✅ Temel Komut Çalıştırma
- Fork-exec mekanizması ile sistem komutlarını çalıştırma
- Tüm standart Linux komutları destekleniyor (ls, pwd, echo, mkdir, touch, cat, grep, vb.)
- Argüman parsing ve komut ayrıştırma

### ✅ Built-in Komutlar
- `cd [dizin]` - Dizin değiştirme (argümansız kullanımda HOME dizinine gider)
- `exit` - Shell'den güvenli çıkış
- `clear` - Terminal ekranını temizleme
- `help` - Komut listesi ve kullanım kılavuzu

### ✅ Koşullu Çalıştırma Operatörleri
- **`&&` (AND)** - Önceki komut başarılıysa (exit code = 0) sonrakini çalıştır
```bash
  mkdir test && cd test && pwd
  gcc program.c -o program && ./program
```
  
- **`||` (OR)** - Önceki komut başarısızsa (exit code ≠ 0) sonrakini çalıştır
```bash
  cd /yokdizin || echo "Dizin bulunamadı"
  make || echo "Derleme hatası!"
```

- **Kombinasyon kullanımı:**
```bash
  mkdir proje && cd proje || echo "Klasör oluşturulamadı"
```

### ✅ Arka Plan Çalıştırma
- **`&`** - Komutu arka planda çalıştır (non-blocking)
```bash
  sleep 10 &
  gcc -o program program.c &
```
- SIGCHLD sinyal yönetimi ile otomatik zombie process temizleme
- Arka plan process'leri için PID gösterimi
- Shell arka planda çalışan process'leri beklemeden devam eder

### 🎨 Kullanıcı Arayüzü
- **Renkli prompt** - Kullanıcı adı, dizin ve komut işareti renklendirmesi
- **Dinamik dizin gösterimi** - Aktif dizin otomatik güncellenir
- **Profesyonel başlangıç ekranı** - ASCII art ile karşılama mesajı
- **Renkli hata mesajları** - Hatalar kırmızı, bilgiler sarı renkte

## 🛠️ Derleme ve Çalıştırma

### Gereksinimler
- GCC derleyici
- Linux/Unix işletim sistemi (Ubuntu, Debian, Fedora, vb.)
- POSIX uyumlu terminal

### Derleme
```bash
gcc shell.c -o myshell
```

veya debug modunda:
```bash
gcc -Wall -Wextra -g shell.c -o myshell
```

### Çalıştırma
```bash
./myshell
```

## 📖 Kullanım Örnekleri

### Basit Komutlar
```bash
myshell> ls -la
myshell> pwd
myshell> echo "Merhaba Dünya"
myshell> cat dosya.txt
```

### Dizin İşlemleri
```bash
myshell> cd /home
myshell> pwd
myshell> cd ..
myshell> cd        # HOME dizinine git
```

### Koşullu Çalıştırma
```bash
# Başarı durumunda devam et
myshell> ls && pwd
myshell> mkdir test && cd test && touch README.md

# Hata durumunda alternatif çalıştır
myshell> cd /yokdizin || echo "Dizin bulunamadı"
myshell> make || echo "Derleme hatası!"

# Karmaşık kombinasyonlar
myshell> mkdir proje && cd proje && touch main.c || echo "İşlem başarısız"
```

### Arka Plan Çalıştırma
```bash
# Uzun süren işleri arka planda çalıştır
myshell> sleep 10 &
[12345] sleep

# Derleme işlemlerini arka planda yap
myshell> gcc -o program program.c &
[12346] gcc

# Arka planda çalışırken başka komutlar
myshell> sleep 30 &
[12347] sleep
myshell> ls
myshell> pwd
# sleep hala arka planda çalışıyor
```

### Built-in Komutlar
```bash
myshell> help      # Yardım menüsünü göster
myshell> clear     # Ekranı temizle
myshell> exit      # Shell'den çık
```

## 🔧 Teknik Detaylar

### Kullanılan Sistem Çağrıları
- **`fork()`** - Yeni process oluşturma (process klonlama)
- **`execvp()`** - Program çalıştırma (process'i değiştirme)
- **`waitpid()`** - Child process bekleme ve exit code alma
- **`chdir()`** - Çalışma dizinini değiştirme
- **`signal()`** - Sinyal yönetimi (SIGCHLD handling)
- **`getcwd()`** - Mevcut dizini öğrenme
- **`getenv()`** - Environment variable okuma

### Sinyal Yönetimi
- **SIGCHLD sinyali** yakalanarak zombie process'ler otomatik temizleniyor
- Arka plan process'leri düzgün yönetiliyor
- Non-blocking process yönetimi ile shell responsive kalıyor

### Komut Parsing
- Operatörler (`&&`, `||`, `&`) doğru şekilde parse ediliyor
- Boşluklar ve özel karakterler işleniyor
- Argümanlar `strtok()` ile ayrıştırılıyor
- Maksimum 64 argüman desteği

### Exit Code Yönetimi
- Her komutun exit code'u yakalanıyor
- `&&` ve `||` operatörleri exit code'a göre karar veriyor
- 0 = Başarılı, 0 dışı = Hata
- `WIFEXITED()` ve `WEXITSTATUS()` makroları ile kod analizi

### Renklendirme
ANSI escape kodları ile terminal renklendirme:
- **Yeşil (32)** - Kullanıcı adı
- **Mavi (34)** - Dizin yolu
- **Sarı (33)** - Komut işareti ve arka plan bilgileri
- **Kırmızı (31)** - Hata mesajları
- **Cyan (36)** - Başlangıç mesajı ve başlıklar

## 📁 Proje Yapısı
```
mini_shell/
├── shell.c       # Ana kaynak kod (~250 satır)
└── README.md     # Proje dokümantasyonu
```

## 🎓 Öğrenilen Kavramlar

Bu proje ile şu konular öğrenildi:
- **Process yönetimi** (fork-exec modeli)
- **Inter-process communication** (parent-child iletişimi)
- **Sinyal işleme** (signal handling)
- **Sistem programlama** (POSIX API kullanımı)
- **String parsing** ve manipülasyon
- **Shell semantics** (komut yorumlama)
- **Exit code** yönetimi ve hata işleme
- **Non-blocking I/O** (arka plan process'leri)

## 🚀 Gelecek Geliştirmeler

Potansiyel iyileştirmeler:
- [ ] Pipe (`|`) desteği
- [ ] Input/Output redirection (`>`, `<`, `>>`)
- [ ] Ctrl+C sinyal yönetimi (SIGINT)
- [ ] Komut geçmişi (history)
- [ ] Tab completion
- [ ] Job control (fg, bg, jobs)
- [ ] Environment variable yönetimi (export)
- [ ] Alias desteği

## 👨‍💻 Geliştirici

- **Emin Dinç** - [@emindinc](https://github.com/emindinc)

## 📝 Lisans

Bu proje eğitim amaçlı geliştirilmiştir.

## 💡 Notlar

- Shell başlatıldığında `help` komutu ile yardım alınabilir
- `exit` komutu ile güvenli çıkış yapılabilir
- Hata mesajları anlaşılır şekilde gösterilir
- Arka plan process'leri otomatik temizlenir (zombie yok!)
- Komutlar büyük/küçük harf duyarlıdır

## 🐛 Bilinen Sınırlamalar

- Maksimum komut uzunluğu: 1024 karakter
- Maksimum argüman sayısı: 64
- Pipe ve redirection henüz desteklenmiyor
 - Ctrl+C shell'i kapatmaz; Ctrl+C yalnızca çalıştırılan child process'lere iletilir

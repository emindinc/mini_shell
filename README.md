# Mini Shell

İşletim Sistemleri dersi için geliştirilmiş, temel shell özellikleri içeren komut yorumlayıcısı.

## Özellikler

### ✅ Temel Komut Çalıştırma
- Fork-exec mekanizması ile sistem komutlarını çalıştırma
- Tüm standart Linux komutları destekleniyor (ls, pwd, echo, mkdir, touch, vb.)

### ✅ Built-in Komutlar
- `cd [dizin]` - Dizin değiştirme
- `exit` - Shell'den çıkış

### ✅ Koşullu Çalıştırma Operatörleri
- `&&` (AND) - Önceki komut başarılıysa sonrakini çalıştır
```bash
  mkdir test && cd test
```
- `||` (OR) - Önceki komut başarısızsa sonrakini çalıştır
```bash
  cd /yokdizin || echo "Dizin bulunamadı"
```

### ✅ Arka Plan Çalıştırma
- `&` - Komutu arka planda çalıştır
```bash
  sleep 10 &
```
- SIGCHLD sinyal yönetimi ile zombie process temizleme

## Derleme ve Çalıştırma

### Derleme
```bash
gcc shell.c -o myshell
```

### Çalıştırma
```bash
./myshell
```

## Kullanım Örnekleri
```bash
# Basit komutlar
myshell> ls -la
myshell> pwd
myshell> echo "Merhaba Dünya"

# Dizin değiştirme
myshell> cd /home
myshell> pwd
myshell> cd ..

# Koşullu çalıştırma
myshell> ls && pwd
myshell> mkdir test && cd test && pwd
myshell> false || echo "Hata oluştu"

# Arka plan çalıştırma
myshell> sleep 5 &
[12345] sleep
myshell> ls    # Sleep arka planda çalışırken başka komut

# Karmaşık kombinasyonlar
myshell> mkdir proje && cd proje && touch README.md && ls -la
myshell> cd /yokdizin || cd /home && pwd
```

## Teknik Detaylar

### Kullanılan Sistem Çağrıları
- `fork()` - Yeni process oluşturma
- `execvp()` - Program çalıştırma
- `waitpid()` - Child process bekleme
- `chdir()` - Dizin değiştirme
- `signal()` - Sinyal yönetimi

### Sinyal Yönetimi
- SIGCHLD sinyali yakalanarak zombie process'ler otomatik temizleniyor
- Arka plan process'leri düzgün yönetiliyor

### Komut Parsing
- Operatörler (`&&`, `||`, `&`) doğru şekilde parse ediliyor
- Boşluklar ve özel karakterler işleniyor
- Argümanlar düzgün ayrıştırılıyor

## Proje Yapısı
```
mini_shell/
├── shell.c       # Ana kaynak kod
└── README.md     # Proje dokümantasyonu
```

## Geliştirici

- **Emin Dinç** - [@emindinc](https://github.com/emindinc)

## Lisans

Bu proje eğitim amaçlı geliştirilmiştir.

## Notlar

- Shell başlatıldığında `exit` komutu ile çıkış yapılabilir
- Ctrl+C ile shell kapanır (gelecek versiyonlarda iyileştirilecek)
- Hata mesajları terminal'e yazdırılır

# 🐚 Mini Shell - Modern Terminal Shell Uygulaması

[![C](https://img.shields.io/badge/Language-C-blue.svg)](https://www.cprogramming.com/)
[![Linux](https://img.shields.io/badge/Platform-Linux-green.svg)](https://www.linux.org/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Modern ve kullanıcı dostu bir terminal shell uygulaması. Renkli arayüz, güçlü komut desteği ve operatör sistemi ile Linux/Unix sistemler için geliştirilmiştir.

![Mini Shell Banner](https://via.placeholder.com/800x200/5B21B6/FFFFFF?text=Mini+Shell+v1.3)

---

## ✨ Özellikler

### 🎨 Görsel Özellikler
- ✅ Renkli ve modern terminal arayüzü
- ✅ Kullanıcı dostu ana menü sistemi
- ✅ Sistem durumu göstergesi (RAM, CPU, Saat)
- ✅ Dinamik prompt (kullanıcı adı + dizin)

### 🔧 Teknik Özellikler
- ✅ 28+ built-in komut
- ✅ Mantıksal operatörler (`&&`, `||`, `&`)
- ✅ Komut geçmişi (200 komut)
- ✅ Sinyal yönetimi (SIGCHLD, SIGINT)
- ✅ Fork-exec ile harici komut desteği
- ✅ Hata kontrolü ve güvenli bellek yönetimi

---

## 📦 Kurulum

### Gereksinimler
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential

# Fedora/RHEL
sudo dnf install gcc make

# Arch Linux
sudo pacman -S base-devel
```

### Derleme
```bash
# Projeyi klonla
git clone https://github.com/kullanici_adi/mini-shell.git
cd mini-shell

# Derle
gcc -Wall -Wextra -o myshell shell.c

# Çalıştır
./myshell
```

---

## 🚀 Kullanım

### Ana Menü
Program başlatıldığında karşınıza gelen menüden seçim yapın:
- **[1] Shell Başlat** - Shell ortamına giriş
- **[2] Güvenli Çıkış** - Programdan çık

### Komut Örnekleri

#### 📁 Dosya ve Dizin İşlemleri
```bash
ls                    # Dosyaları listele
cd /home/user         # Dizin değiştir
pwd                   # Mevcut dizini göster
mkdir yeni_klasor     # Klasör oluştur
rmdir bos_klasor      # Boş klasör sil
touch dosya.txt       # Dosya oluştur
rm dosya.txt          # Dosya sil
cp kaynak.txt hedef.txt   # Dosya kopyala
chmod 755 script.sh   # İzinleri değiştir
```

#### 📄 İçerik İşlemleri
```bash
grep "kelime" dosya.txt   # Kelime ara
tail dosya.txt            # Son 10 satır
```

#### 💻 Sistem Komutları
```bash
calc 5 + 3            # Hesap makinesi
free                  # RAM durumu
df                    # Disk durumu
date                  # Tarih ve saat
whoami                # Kullanıcı adı
rand                  # Rastgele sayı (0-999)
history               # Komut geçmişi
clear                 # Ekranı temizle
```

#### ⚙️ Operatörler
```bash
# AND (&&) - Önceki başarılıysa devam
mkdir test && cd test && pwd

# OR (||) - Önceki başarısızsa devam
cd /yok || echo "Dizin bulunamadi!"

# BACKGROUND (&) - Arka planda çalıştır
sleep 10 &

# Karmaşık zincirleme
mkdir proje && cd proje && touch README.md || echo "Hata!"
```

---

## 📚 Komut Listesi

### Dosya İşlemleri
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `ls` | Dosyaları listele | `ls` |
| `cd` | Dizin değiştir | `cd /home` |
| `pwd` | Mevcut dizin | `pwd` |
| `mkdir` | Klasör oluştur | `mkdir yeni` |
| `rmdir` | Klasör sil | `rmdir bos` |
| `touch` | Dosya oluştur | `touch test.txt` |
| `rm` | Dosya sil | `rm test.txt` |
| `cp` | Kopyala | `cp a.txt b.txt` |
| `chmod` | İzin değiştir | `chmod 755 file` |

### İçerik İşlemleri
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `grep` | Kelime ara | `grep "test" file.txt` |
| `tail` | Son 10 satır | `tail log.txt` |

### Sistem Komutları
| Komut | Açıklama | Örnek |
|-------|----------|-------|
| `calc` | Hesapla | `calc 10 + 5` |
| `free` | RAM durumu | `free` |
| `df` | Disk durumu | `df` |
| `date` | Tarih/Saat | `date` |
| `whoami` | Kullanıcı | `whoami` |
| `rand` | Rastgele sayı | `rand` |
| `history` | Geçmiş | `history` |
| `clear` | Temizle | `clear` |
| `exit` | Çıkış | `exit` |

---

## 🏗️ Mimari

### Kod Yapısı
```
mini-shell/
│
├── shell.c          # Ana kaynak kod (560+ satır)
├── README.md        # Dökümantasyon
├── LICENSE          # Lisans dosyası
└── myshell          # Derlenmiş binary (çalıştırılabilir)
```

### Fonksiyon Organizasyonu
```c
// Sinyal Yönetimi
sigchld_handler()    // Zombie process temizleme
sigint_handler()     // Ctrl+C işleme

// UI Fonksiyonları
show_fancy_menu()    // Ana menü gösterimi
restore_terminal()   // Terminal ayarlarını geri yükle

// Komut İşleme
execute_command()    // Komut çalıştırma motoru
handle_operators()   // Operatör parsing ve mantık
shell_loop()         // Ana shell döngüsü
main()               // Program giriş noktası
```

### İşleyiş Akışı
```
Başlangıç
    ↓
Ana Menü → [1] Shell Başlat → Shell Loop
                                  ↓
                             Komut Oku
                                  ↓
                          Operatör Parse
                                  ↓
                          ┌──────┴──────┐
                    Built-in?         External
                          ↓                ↓
                      Doğrudan         fork-exec
                      Çalıştır         Çalıştır
                          ↓                ↓
                          └──────┬──────┘
                                  ↓
                          Exit Code Döndür
                                  ↓
                             Tekrar → [exit] → Ana Menü
```

---

## 🔒 Güvenlik Özellikleri

- ✅ **Buffer Overflow Koruması**: `strncpy()` kullanımı
- ✅ **Sinyal Güvenliği**: SIGCHLD race condition düzeltildi
- ✅ **Hata Kontrolü**: Tüm sistem çağrılarında kontrol
- ✅ **Bellek Yönetimi**: File descriptor sızıntısı yok
- ✅ **Input Validasyonu**: Komut argümanları kontrol edilir

---

## 🐛 Bilinen Sorunlar ve Çözümler

### Sorun: "Komut bulunamadı" hatası
**Çözüm**: Komutun PATH'te olduğundan emin olun
```bash
which komut_adi
```

### Sorun: İzin hatası
**Çözüm**: Shell'i çalıştırılabilir yapın
```bash
chmod +x myshell
```

### Sorun: Derleme hatası
**Çözüm**: GCC kurulu olduğundan emin olun
```bash
gcc --version
```

---

## 📊 Performans

| Metrik | Değer |
|--------|-------|
| Kaynak Kod | ~560 satır |
| Binary Boyut | ~25 KB |
| Başlangıç Süresi | <100ms |
| Bellek Kullanımı | ~2 MB |
| Komut Geçmişi | 200 komut |

---

## 🛠️ Geliştirme

### Kod Standartları
- **Stil**: K&R C style
- **Derleyici**: GCC 9.0+
- **Standart**: C99
- **Uyarılar**: `-Wall -Wextra` ile temiz

### Test Senaryoları
```bash
# Temel komutlar
./myshell
> ls
> pwd
> cd /tmp

# Operatörler
> mkdir test && cd test && pwd
> false || echo "Çalıştı!"

# Hata durumları
> cd /yokdizin
> chmod 999 dosya.txt

# Çıkış
> exit
```

### Katkıda Bulunma
```bash
# Fork'la
git clone https://github.com/senin-kullanici/mini-shell.git

# Branch oluştur
git checkout -b yeni-ozellik

# Commit yap
git commit -am "Yeni özellik eklendi"

# Push et
git push origin yeni-ozellik

# Pull Request aç
```

---

## 📝 Değişiklik Geçmişi

### v1.3 (Mevcut)
- ✅ Alternate screen kaldırıldı
- ✅ Hata mesajları düzeltildi
- ✅ Terminal temizleme iyileştirildi
- ✅ Prompt düzeltmeleri

### v1.2
- ✅ SIGCHLD race condition düzeltildi
- ✅ Exit code yönetimi iyileştirildi
- ✅ Buffer overflow koruması eklendi
- ✅ system() çağrıları kaldırıldı

### v1.1
- ✅ Operatör sistemi (`&&`, `||`, `&`)
- ✅ 28 built-in komut
- ✅ Komut geçmişi
- ✅ Renkli arayüz

### v1.0
- 🎉 İlk sürüm

---

## 📄 Lisans

Bu proje MIT Lisansı altında lisanslanmıştır. Detaylar için [LICENSE](LICENSE) dosyasına bakın.
```
MIT License

Copyright (c) 2025 [Senin Adın]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

---

## 👥 Katkıda Bulunanlar

- **[Senin Adın]** - *Proje Sahibi* - [GitHub](https://github.com/kullanici_adi)

---

## 🙏 Teşekkürler

Bu proje aşağıdaki kaynaklardan ilham almıştır:
- [Bash](https://www.gnu.org/software/bash/)
- [Zsh](https://www.zsh.org/)
- [Advanced Programming in the UNIX Environment](https://www.apuebook.com/)

---

## 📞 İletişim

- 💼 LinkedIn: [Profil](https://www.linkedin.com/in/muhammed-emin-dinc/)

---

## ⭐ Yıldız Ver!

Bu projeyi beğendiyseniz, GitHub'da ⭐ vermeyi unutmayın!

---

**Made with ❤️ using C**

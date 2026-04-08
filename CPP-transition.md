# C++ Geçiş Tartışması

## Neden Bu Tartışma?

Blusys HAL ve servis katmanları tamamlandı. Uygulama katmanı (`app/`) ilk kez şekillenirken — UI sistemi, state machine'ler, device manager, config — dil tercihini netleştirme zamanı. Yanlış noktada verilen bu karar sonradan geri alınması zor bir borç bırakır.

**Mevcut durum:** HAL + servisler tamamen C. Uygulama katmanı henüz yazılmadı.

**Soru:** Uygulama katmanı C++ ile mi yazılmalı, yoksa C tutarlılığı korunmalı mı?

---

## C++ Lehine Argümanlar

### Derleyici tarafından zorlanan kapsülleme
C'de opak handle ile kapsülleme yapılabilir ama başlık dahil edilirse struct'a erişim engellenemez. C++'da `private:` gerçek bir duvar — derleyici zorlar.

### Namespace ile organizasyon
Birden fazla ürün katmanı aynı isim alanında büyüdükçe prefix çakışmaları olur. `ui::button::primary_create()` vs `ui_button_primary_create()` — fark kozmetik görünür ama büyük kod tabanında arama ve okuma kalitesini etkiler.

### State machine okunabilirliği
Karmaşık state machine'ler C++'da class + enum class + method ile daha net ifade edilebilir. Fonksiyon pointer tabloları da işe yarar ama C++'ın tip güvenliği hata yakalamayı erkene alır.

### Gelecekte çoklu ürün karmaşıklığı
Ürün sayısı arttıkça paylaşılan uygulama katmanı kodu büyür. C++ abstract interface'leri (ürüne özgü davranışı polimorfizm ile enjekte etmek) bu geçişi temiz tutar.

---

## C++ Aleyhine Argümanlar / Endişeler

### Spaghetti'yi dil çözmez, mimari çözer
C++'a geçmek kötü organizasyonu temiz yapmaz — sadece daha karmaşık hale getirir. Katmanlı mimari + ürün scaffold disiplini, dil değişikliğinden daha büyük koruma sağlar.

### LVGL RAII tuzağı
LVGL kendi nesne ağacını yönetir: ebeveyn silinince çocuklar da silinir. C++ destructor'da `lv_obj_delete` çağırmak çift silme (double-free) riskine girer. RAII argümanı LVGL için sandığından zayıf.

### 4 MB flash / 512 KB SRAM kısıtı
SRAM gerçek darboğaz. C++ geçici değişkenler, global constructor zinciri ve şablon genişlemesi SRAM'ı sessizce yer. `-fno-exceptions -fno-rtti` zorunlu ama yeterli değil — disiplin de şart.

### ESP-IDF C ekosistemi
Her forum cevabı, Espressif örneği, LVGL issue ve topluluk kaynağı C. C++ crash'ini C framework içinde debug etmek ekstra mental yük.

### Disiplin duvarları görünmez
`-fno-exceptions` derleyici tarafından zorlanır. "Kalıcı üye olarak `std::vector` kullanma" zorlanmaz. Yorgun bir anda yazılan bir satır firmware'i şişirebilir. C'nin sınırları daha sert.

---

## Spaghetti Problemi: Gerçek Çözüm

Çok ürünlü kod tabanında spaghetti'nin asıl kaynakları:
- "Bu kod nereye gider?" sorusunun net cevabı yok
- Business logic UI callback'lerine sızıyor
- Ortak state her yerden erişilebiliyor
- Her ürün kendi yapısını sıfırdan icat ediyor

**Çözüm: Scaffold + katman sözleşmesi**, dil değil.

```
app/
  ui/          → yalnızca widget ve ekran composition
  controllers/ → business logic, state machine
  services/    → servis başlatma ve yaşam döngüsü
  config/      → settings store
```

Bu yapı bir kez tanımlanır, her yeni ürün aynı iskeletle başlar. Katman sözleşmesi (`ui/` doğrudan servis çağıramaz; controller üzerinden gider) CMakeLists.txt `REQUIRES` ile zorlanabilir.

---

## Eğer Geçiş Yapılırsa — Zorunlu Kısıtlar

Bu kısıtlar olmadan C++ 4 MB / 512 KB hedef için uygun değil:

```cmake
target_compile_options(app PRIVATE -fno-exceptions -fno-rtti -Os)
```

| Kural | Neden |
|-------|-------|
| `new` / `delete` yok — LVGL nesneleri için `lv_malloc` / `lv_free` | SRAM kontrolü |
| Kalıcı üye olarak `std::string`, `std::vector` yok | Gizli heap allocasyon |
| Hot path'te virtual destructor yok | vtable maliyeti |
| Template sadece gerçek duplikasyonu ortadan kaldırdığı yerde | Binary şişmesi |
| C++17 izin verilir (`if constexpr`, `std::optional`, structured binding) | Faydalı, düşük maliyet |
| `std::function` yok — fonksiyon pointer veya template callback | Her çağrıda heap |

---

## Karar Matrisi

| Kriter | C | C++ |
|--------|---|-----|
| Mevcut kod tabanıyla tutarlılık | ✓ | — |
| ESP-IDF ekosistemi uyumu | ✓ | — |
| Derleyici zorlanan kapsülleme | ~ (opak handle) | ✓ |
| SRAM güvenliği | ✓ | Dikkat gerekir |
| Debug kolaylığı | ✓ | — |
| State machine netliği | ~ | ✓ |
| Çok ürün ölçeklenebilirliği | ✓ (scaffold ile) | ✓ |
| LVGL RAII | Gereksiz | Risk taşır |

---

## Şu Anki Durum

**Karar verilmedi.** Bu belge açık bir tartışmayı belgeler; bir sonuç kaydetmez.

Tartışılan iki taraf da geçerli argümanlara sahip. Henüz netleşmeyen sorular:

- Çok ürünlü uygulama katmanı gerçekte ne kadar karmaşık olacak?
- Scaffold + katman sözleşmesi C'de yeterli mi, yoksa derleyici zorlanan sınırlara gerçekten ihtiyaç duyulacak mı?
- SRAM baskısı pratikte C++'ı dışlıyor mu, yoksa kısıtlarla yönetilebilir mi?

---

## Kararı Netleştirecek Tetikleyiciler

Aşağıdakilerden biri yaşandığında bu belgeye dönüp karar gözden geçirilmeli:

1. Ürün sayısı arttıkça paylaşılan uygulama kodu yönetilemez hale gelirse
2. State machine karmaşıklığı C fonksiyon pointer tablolarını sürdürülemez kılarsa
3. Katman sınırlarının derleyici tarafından zorlanmasına ihtiyaç duyulursa
4. İlk ürün uygulaması yazıldıktan sonra scaffold'un tuttuğu ve tutmadığı yerler netleşirse

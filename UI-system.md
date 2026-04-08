# Embedded UI / Design System Yolu

## Amaç
Tek tek ekran çizmek yerine ortak bir **UI iskeleti** kurmak:
- yeni ürün çıkarmayı hızlandırmak
- ürünler arasında tutarlılık sağlamak
- UI kodunu temiz ve tekrar kullanılabilir tutmak

## Stack
- **ESP-IDF**
- **C** → tüm katmanlar (HAL, services, uygulama, UI); dil tutarlılığı bilinçli bir tercih
- **LVGL** → UI runtime
- **Tasarım aracı** → Figma ve benzeri (Penpot, Lunacy, …); referans + değişkenler
- **JSON** → repodaki token / tema verisi; `scripts/gen_theme.py` ile `theme.h` üretilir
- **PC'de LVGL + SDL2** → hızlı UI iterasyonu (kart gerekmez)
- **ESP-IDF QEMU** → firmware doğrulaması (kart gerekmez)

## Ana Prensip
**Tasarım referansı olsun, gerçek sistem kodda yaşasın.**

**Source of truth (öncelik sırası):**
1. `app/ui/` kodu (bileşenler, ekran composition)
2. `theme.json` → `theme.h` (token / tema tanımları)
3. Tasarım aracı — görsel ve isimlendirme sözleşmesi (piksel kaynağı değil)

## Repo Sınırları

```
blusys repo  (bu repo — değişmez)
  components/blusys/           ← C, HAL
  components/blusys_services/  ← C, servisler (blusys_ui dahil: LVGL init + lock)
  examples/                    ← C, API demoları

ürün repo  (ayrı repo)
  main/app_main.c
  app/
    ui/
      tokens/    theme.h       ← scripts/gen_theme.py çıktısı; commit edilmez
      components/              ← button, row, card, modal, badge, input
      patterns/                ← page_shell, settings_section, status_panel
      screens/                 ← sadece composition
      overlays/                ← toast, confirm_modal, spinner
    controllers/               ← state machine, event handler
    services/                  ← device_manager, orchestrasyon
    config/                    ← settings store
  scripts/
    gen_theme.py               ← theme.json → theme.h üretici
  CMakeLists.txt               ← EXTRA_COMPONENT_DIRS = ../../blusys/components
```

## `blusys_ui` — LVGL Çalışma Zamanı Temeli

`blusys_services` içindeki `blusys_ui` modülü **değişmez ve dokunulmaz**:
- LVGL init + render task (`blusys_ui_open / blusys_ui_close`)
- LCD'ye çift tamponlu flush (`blusys_lcd_draw_bitmap`)
- Thread-safe widget erişimi (`blusys_ui_lock / blusys_ui_unlock`)

Ürün reposundaki `app/ui/` bu modülün **üzerine** kurulur; onu değiştirmez veya kopyalamaz.

## Token Pipeline

```
Figma / Penpot  →  manuel export  →  theme.json  (repoda, kaynak)
                                           ↓
                                   scripts/gen_theme.py
                                           ↓
                                   app/ui/tokens/theme.h  (üretilen, commit edilmez)
```

`theme.json` şeması:
```json
{
  "color":   { "primary": "#2196F3", "surface": "#1E1E1E", "on_primary": "#FFFFFF" },
  "spacing": { "sm": 8, "md": 16, "lg": 24 },
  "radius":  { "card": 8, "button": 4 },
  "font":    { "body": "lv_font_montserrat_14", "title": "lv_font_montserrat_20" }
}
```

Üretilen `theme.h`:
```c
/* AUTO-GENERATED — theme.json kaynak dosyasını düzenleyin */
#pragma once
#include "lvgl.h"

#define UI_COLOR_PRIMARY      lv_color_hex(0x2196F3)
#define UI_COLOR_SURFACE      lv_color_hex(0x1E1E1E)
#define UI_COLOR_ON_PRIMARY   lv_color_hex(0xFFFFFF)

#define UI_SPACING_SM   8
#define UI_SPACING_MD   16
#define UI_SPACING_LG   24

#define UI_RADIUS_CARD    8
#define UI_RADIUS_BUTTON  4

#define UI_FONT_BODY    (&lv_font_montserrat_14)
#define UI_FONT_TITLE   (&lv_font_montserrat_20)
```

## Katmanlar

| Katman | Konum | İçerik |
|--------|-------|--------|
| **tokens** | `ui/tokens/theme.h` | renk, spacing, radius, font (JSON'dan üretilir) |
| **components** | `ui/components/` | button, row, card, badge, input, modal |
| **patterns** | `ui/patterns/` | page_shell, settings_section, status_panel |
| **screens** | `ui/screens/` | sadece composition — ham LVGL çağrısı yok |
| **overlays** | `ui/overlays/` | toast, confirm_modal, spinner |
| **controllers** | `app/controllers/` | event + business logic |
| **services** | `app/services/` | device_manager, orchestrasyon |
| **config** | `app/config/` | settings store |

## Kod İlkesi

Ekran kodu böyle olmalı:

```c
ui_button_primary_create(parent, "OK", on_press);
ui_row_setting_create(parent, &cfg);
ui_modal_confirm_show("Emin misiniz?", on_confirm);
```

Böyle olmamalı (bunlar bileşenlerin içinde kalsın):

```c
lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
lv_obj_set_style_radius(btn, UI_RADIUS_BUTTON, 0);
lv_obj_set_style_text_color(label, UI_COLOR_ON_PRIMARY, 0);
```

Bileşen örneği:

```c
/* ui/components/ui_button.c */
lv_obj_t *ui_button_primary_create(lv_obj_t *parent,
                                   const char *label,
                                   lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_style_bg_color(btn, UI_COLOR_PRIMARY, 0);
    lv_obj_set_style_radius(btn, UI_RADIUS_BUTTON, 0);
    /* ... */
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}
```

## LVGL Thread Safety

`blusys_ui_lock / blusys_ui_unlock` render task dışından widget oluştururken veya güncellerken **her zaman** kullanılmalıdır:

```c
blusys_ui_lock(ui);
ui_button_primary_create(screen, "OK", on_press);
blusys_ui_unlock(ui);
```

Render task içinden (LVGL callback'leri) çağrı yaparken lock gerekmez — render task zaten sahibidir.

## Uygulama Akışı

1. `theme.json` tasarım aracıyla veya elle hizala.
2. `scripts/gen_theme.py` ile `theme.h` üret; bunu `CMakeLists.txt` build adımına ekle.
3. `app/ui/components/` bileşenlerini yaz; tüm LVGL çağrıları burada toplansın.
4. `app/ui/screens/` sadece bileşenleri çağırsın; `lv_obj_set_style_*` görmemeli.
5. Arayüzü [üç doğrulama yolundan](#ui-validation-paths) uygun olanla test et.

## Üç Doğrulama Yolu {#ui-validation-paths}

| # | Katman | Ne doğrulanır | Kart gerekir mi? |
|---|--------|----------------|------------------|
| **1** | **PC'de LVGL + SDL2** | Widget, layout, tema; en hızlı UI iterasyonu. ESP firmware değil; blusys/ESP init stub. | Hayır |
| **2** | **ESP-IDF QEMU** `idf.py qemu --graphics` | ESP32 olarak derlenen firmware, FreeRTOS, render task, `blusys_ui_lock` akışı. | Hayır |
| **3** | **Gerçek donanım** | Zamanlama, SRAM baskısı, gerçek panel, dokunma, güç; nihai referans. | Evet |

**Özet:** PC → QEMU → kart. Kart atmadan önce iki kartsız doğrulama katmanı geç.

## Ürün Scaffold İlkesi

Her yeni ürün aynı `app/` iskeletini kopyalayarak başlar. Mimari kararlar bir kez verilir, her ürün için yeniden icat edilmez:

- `app/ui/` → yalnızca ekranlar ve bileşenler
- `app/controllers/` → business logic, state machine
- `app/services/` → servis başlatma ve yaşam döngüsü
- `app/config/` → ayarlar

Bileşenler ve pattern'ler ürünler arasında taşınabilir; `theme.json` değişince tüm ürünler otomatik güncellenir.

## Araçlar

- **Varsayılan:** elle yazılmış `app/ui/` + JSON token hattı.
- **SquareLine:** hızlı mock / demo; ana mimari kaynak değil.
- **LVGL Pro / Figma eklentileri:** isteğe bağlı; çıktı her zaman gözden geçirilmeli.

## Sonuç

```
Tasarım referansı (Figma/Penpot)
  → theme.json  (token source of truth)
  → gen_theme.py → theme.h
  → app/ui/components/ (LVGL çağrıları burada)
  → app/ui/screens/ (sadece composition)
  → PC+SDL2 / QEMU+graphics / kart ile doğrula
```

Bu yapı prototipi hızlandırır, ürünler arasında tutarlılık sağlar ve LVGL detayını tek katmanda toplar.

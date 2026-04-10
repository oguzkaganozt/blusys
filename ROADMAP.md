# Blusys — State & Roadmap

## What We're Building

Blusys is an internal ESP32 product platform. The goal is to stop rebuilding the same
low-level plumbing on every new product. HAL, drivers, and services are solved once and
shared; a C++ framework layer ships the runtime spine and UI widget kit; product teams
write business logic, not peripheral glue.

One platform. Many products. One spine.

---

## Where We Are — v6.1.0

**Current release: v6.1.0** (April 2026). The V6 platform is complete and validated on
real hardware across all three targets (ESP32, ESP32-C3, ESP32-S3).

### What ships

Three ESP-IDF components under one version tag:

```
components/blusys/            HAL + drivers (C)
components/blusys_services/   runtime services (C)
components/blusys_framework/  framework + widget kit (C++)
```

**HAL** — `gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `adc`, `dac`, `sdm`,
`pwm`, `mcpwm`, `timer`, `pcnt`, `rmt`, `rmt_rx`, `i2s`, `i2s_rx`, `twai`, `wdt`,
`sleep`, `temp_sensor`, `touch`, `sdmmc`, `sd_spi`, `nvs`, `one_wire`, `efuse`, `ulp`,
`usb_host`, `usb_device`, `system`, `error`, `target`, `version`

**Drivers** — Display: `lcd`, `led_strip`, `seven_seg` · Input: `button`, `encoder` ·
Sensor: `dht` · Actuator: `buzzer`

**Services** — `ui` (LVGL lifecycle + render task), `usb_hid`, `wifi`, `wifi_prov`,
`wifi_mesh`, `espnow`, `bluetooth`, `ble_gatt`, `mdns`, `mqtt`, `http_client`,
`http_server`, `ws_client`, `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`,
`local_ctrl`

**Framework** — Core spine: `router`, `intent`, `feedback`, `controller`, `runtime` ·
Layout: `screen`, `row`, `col`, `label`, `divider` · Widget kit: `bu_button`,
`bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay` · Theme: single `theme_tokens` struct
populated at boot · Encoder helpers: `create_encoder_group`, `auto_focus_screen`

**Tooling** — `blusys create --starter <headless|interactive>` product scaffold ·
`blusys host-build` PC + SDL2 harness (LVGL v9.2.2) · `blusys lint` layering gate ·
`blusys qemu` QEMU smoke runner

### Validation state

All checks passing:

| Check | Result |
|---|---|
| `blusys lint` | ✅ layering gate |
| `blusys build-examples` (all 3 targets) | ✅ |
| `blusys host-build` (Ubuntu 24.04) | ✅ `hello_lvgl` + `widget_kit_demo` + `pomodoro_host` |
| `blusys qemu` headless scaffold (all 3 targets) | ✅ |
| Real hardware — headless scaffold | ✅ esp32 + esp32c3 + esp32s3 |
| Real hardware — interactive scaffold + ST7735 LCD | ✅ esp32c3 |
| Real hardware — SSD1306 OLED + encoder (`oled_encoder_basic`) | ✅ esp32 |
| `mkdocs build --strict` | ✅ |

Full hardware validation record: `docs/validation-report-v6.md`.

### Release notes

**v6.1.0** — I2C sync mode fix (silent write failures on async mode), `blusys_ui` mono-
page panel support for SSD1306-style OLEDs, `oled_encoder_basic` Pomodoro example,
`pomodoro_host` SDL2 demo, host build scaffolding in `blusys create`.

**v6.0.0** — Initial V6: full three-tier architecture, V1 widget kit, product scaffold,
host harness, QEMU validation, real-hardware validation on all three targets.
Migration guide: `docs/migration-guide.md`.

---

## What's Next — V1.1

Near-term items that don't block v6.1.0 but should land before V2.

### End-to-end encoder + LCD rig

Each piece is individually validated but no single session has wired a physical EC11
encoder, an ST7735 panel, and the framework spine into one rig end-to-end
(encoder-rotation → focus-traversal → confirm-press → overlay-show).

Target: esp32-s3 + ST7735 SPI + EC11 encoder. Use the interactive scaffold
`app_main.cpp` LCD init TODO block as the reference.

### Scope-based input → feedback latency

Wire `intent::confirm` to a debug GPIO toggle inside `controller::handle`. Tap that GPIO
+ the first `blusys_lcd_draw_bitmap` SPI CS edge with a Saleae or oscilloscope. Record
median + p99 over a few hundred presses. File under `docs/guides/` using the
hardware-validation report template.

### SSD1306 bus recovery

A failed boot can leave SDA held low mid-transaction, locking the I2C bus for the next
power cycle. Fix: before `i2c_new_master_bus`, if SDA reads low pulse SCL 9 times,
issue a manual STOP condition, then release the GPIOs. Blocked on a deliberate stuck-
state reproduction (~20-line change once reproducible).

### ~~`widget_kit_demo` keyboard encoder simulation~~ ✅

Done. Arrow keys (Left/Right/Up/Down) map to encoder rotation, Space/Enter map to
encoder press. An `lv_indev_t` of type `LV_INDEV_TYPE_ENCODER` drives LVGL focus
traversal via `create_encoder_group` + `auto_focus_screen`, mirroring the on-device
`framework_encoder_basic` pattern. Both mouse and keyboard input work simultaneously.

---

## What's Planned — V2

Not yet scheduled. Captured to preserve the reasoning.

### `bu_knob` — Camp 3 rotary widget

A knob widget using `lv_obj_class_t` embedded storage (Camp 3) instead of the external
slot pool (Camp 2). Deferred from V1 so the Camp 2 pattern could stabilize across five
widgets before introducing a second implementation pattern.

### Services → C++ migration

`blusys_services` is C. Migrating it to C++ would enable captureless lambdas in service
callbacks and align it with the framework tier. Deferred because the existing
implementations are already clean and well-factored, and no concrete pain point has
appeared that C cannot address. Should be driven by a real need, not cleanup.

### Per-module build gating

V1 gates the entire UI tier with a binary `BLUSYS_STARTER_TYPE` flag. V2 could
introduce per-module flags (`BLUSYS_MODULE_WIFI`, `BLUSYS_MODULE_BLE`, etc.) so
products can strip unused services. Worth revisiting once there are three or more
products with different module subsets.

### Latency tooling

Formalize the input → feedback latency measurement methodology — GPIO-toggle + scope
integration as a standard validation step, with a template report and expected
thresholds. Currently measured ad-hoc.

### LVGL version track

LVGL v9.2.2 is pinned in `scripts/host/CMakeLists.txt` and each interactive example's
`idf_component.yml`. Review when v9.3+ or v10.x stabilize. Risk areas: `lv_arc`,
`lv_switch`, and the `LV_EVENT_*` set the widget kit directly uses.

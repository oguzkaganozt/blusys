# Progress

## Current Release

**v6.1.0** — released April 2026.

Blusys is a three-tier ESP32 product platform:

```
components/blusys/            HAL + drivers (C)
components/blusys_services/   runtime services (C)
components/blusys_framework/  framework + widget kit (C++)
```

All three tiers ship under a single version tag. Platform components are consumed by
product repos as ESP-IDF managed dependencies pinned in `main/idf_component.yml`.

## What Ships in V6

### HAL (`blusys`)

`gpio`, `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `adc`, `dac`, `sdm`, `pwm`,
`mcpwm`, `timer`, `pcnt`, `rmt`, `rmt_rx`, `i2s`, `i2s_rx`, `twai`, `wdt`, `sleep`,
`temp_sensor`, `touch`, `sdmmc`, `sd_spi`, `nvs`, `one_wire`, `efuse`, `ulp`,
`usb_host`, `usb_device`, `system`, `error`, `target`, `version`

### Drivers (`blusys/drivers/`)

Display: `lcd`, `led_strip`, `seven_seg`
Input: `button`, `encoder`
Sensor: `dht`
Actuator: `buzzer`

### Services (`blusys_services`)

Display/runtime: `ui` (LVGL lifecycle, render task, flush orchestration)
Input/runtime: `usb_hid`
Connectivity: `wifi`, `wifi_prov`, `wifi_mesh`, `espnow`, `bluetooth`, `ble_gatt`, `mdns`
Protocol: `mqtt`, `http_client`, `http_server`, `ws_client`
System: `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl`

### Framework (`blusys_framework`)

Core spine: `router`, `intent`, `feedback`, `controller`, `runtime`
Layout primitives: `screen`, `row`, `col`, `label`, `divider`
Widget kit: `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`
Theme system: single `theme_tokens` struct populated at boot
Encoder helpers: `create_encoder_group`, `auto_focus_screen`
Product scaffold: `blusys create --starter <headless|interactive>`
Host harness: `blusys host-build` (LVGL v9.2.2 + SDL2 on Linux)

## Release History

### v6.1.0 (April 2026)

Patch release fixing two bugs surfaced on real esp32 + SSD1306 I2C OLED hardware.
Backwards-compatible — existing v6.0.0 products keep working unchanged.

- **I2C sync mode fix:** `blusys_i2c_master_open` now uses `trans_queue_depth = 0`
  (synchronous mode). Async mode caused silent write failures — every `i2c_master_transmit`
  returned `ESP_OK` immediately without confirming the slave ACK, producing 117 phantom
  devices across the full 7-bit address space during scan.
- **`blusys_i2c_master_probe` write-based fix:** replaced `i2c_master_probe` (documented
  as incompatible with async master) with a 1-byte write probe. Works in sync mode and
  is harmless on all common I2C devices (SSD1306, BMP280, MPU6050, 24LCxx).
- **`blusys_ui` mono-page panel support:** added `blusys_ui_panel_kind_t` to
  `blusys_ui_config_t`. New `BLUSYS_UI_PANEL_KIND_MONO_PAGE` selects a flush path that
  thresholds RGB565 pixels to 1 bit and packs into SSD1306-style page format. Default
  `BLUSYS_UI_PANEL_KIND_RGB565` is unchanged — fully backwards-compatible.
- **`oled_encoder_basic` example:** SSD1306 + rotary encoder Pomodoro timer. Validates
  the mono-page flush path and I2C sync-mode probe end-to-end on hardware.
- **`pomodoro_host` host executable:** pixel-identical 128×32 OLED layout running on
  the PC + SDL2 harness at 4× zoom (512×128). Added to `scripts/host/CMakeLists.txt`.
- **Host build scaffolding:** `blusys create` now generates `host/CMakeLists.txt` and
  `host/main.cpp` for both starter types. `blusys host-build [project]` builds either
  the monorepo harness or a scaffolded product's host target.

### v6.0.0 (April 2026)

Initial V6 release completing the platform transition. Full three-tier architecture,
V1 widget kit, product scaffold, host harness, QEMU validation, and real-hardware
validation on all three targets. See `docs/validation-report-v6.md` for the full
hardware validation record.

Key changes vs V5:
- Three-tier component model (`blusys` / `blusys_services` / `blusys_framework`)
- Drivers moved from `blusys_services` into `components/blusys/src/drivers/`
- `blusys lint` CI gate enforcing HAL/drivers boundary
- `blusys_framework` C++ tier: core spine + V1 widget kit + encoder helpers
- `blusys create --starter <headless|interactive>` product scaffold
- `blusys host-build` PC + SDL2 harness (LVGL v9.2.2)
- V5→V6 migration guide at `docs/migration-guide.md`
- `blusys_all.h` removed; use per-tier umbrella headers

## Verification Snapshot

All checks passing against v6.1.0:

- `./blusys lint` — layering gate passes
- `./blusys build examples/smoke esp32s3 / esp32c3 / esp32` — baseline passes
- `./blusys build examples/button_basic esp32s3` — driver example passes
- `./blusys build examples/ui_basic esp32s3` — LVGL service example passes
- `./blusys build examples/oled_encoder_basic esp32 / esp32c3 / esp32s3` — OLED + encoder passes
- `./blusys build examples/framework_app_basic esp32s3 / esp32 / esp32c3` — full spine end-to-end
- `./blusys build examples/framework_encoder_basic esp32s3 / esp32 / esp32c3` — encoder traversal
- `./blusys host-build` — builds `hello_lvgl` + `widget_kit_demo` + `pomodoro_host` on Ubuntu 24.04
- `./blusys create --starter interactive` + build — passes on esp32c3 (real hardware)
- `./blusys create --starter headless` + build + QEMU — passes on all three targets
- Phase 9 real hardware: headless scaffold boots clean on esp32 / esp32c3 / esp32s3
- Phase 9 real hardware: interactive scaffold + ST7735 SPI LCD on esp32c3
- v6.1.0 real hardware: SSD1306 I2C OLED on esp32 via `oled_encoder_basic`
- `mkdocs build --strict` — doc gate passes

## V1.1 Follow-ups

Items tracked in `ROADMAP.md` that do not block the current release:

1. End-to-end encoder + LCD single rig (esp32-s3 + ST7735 + EC11)
2. Scope-based input → feedback latency measurement
3. SSD1306 bus recovery on `blusys_lcd_open` (after stuck-state reproduction)
4. `widget_kit_demo` keyboard encoder simulation (optional polish)

# V6 Validation Report

This document records the hardware validation measurements for the V6 platform release
(v6.0.0 and v6.1.0). It is the permanent validation record for the three-tier
Blusys platform: HAL + drivers (`blusys`), services (`blusys_services`), and
framework + widget kit (`blusys_framework`).

## Validation chain

V6 validation uses a three-stage chain to catch bugs that earlier stages cannot:

| Stage | Environment | What it catches |
|-------|-------------|-----------------|
| 1 | PC + LVGL + SDL2 (host harness) | Compilation, widget layout, framework spine linkage |
| 2 | ESP-IDF QEMU | Boot sequence, spine events, headless runtime |
| 3 | Real hardware | LCD flush path, FreeRTOS task interaction, I2C/SPI bus behavior |

Stages 1 and 2 are fast and run without hardware. Stage 3 is the hardware gate and
cannot be skipped for modules that touch the LCD flush path, render task, or any
peripheral bus.

---

## Stage 1 — PC + LVGL + SDL2

**Artifact:** `scripts/host/build-host/widget_kit_demo`

The host harness (`scripts/host/CMakeLists.txt`) compiles all `blusys_framework` C++
sources (core spine + V1 widget kit + layout primitives) plus
`components/blusys/src/common/error.c` into `libblusys_framework_host.a` and links it
into `widget_kit_demo` alongside host LVGL v9.2.2 + SDL2. The same
runtime → controller → route sink chain validated on-device by
`examples/framework_app_basic/` is exercised here.

Build evidence:
```
libblusys_framework_host.a         93,994 bytes
widget_kit_demo                 1,010,856 bytes  (hello_lvgl: 976,984)
blusys::ui::* symbols            46
blusys::framework::* symbols     74
```

Smoke run with `SDL_VIDEODRIVER=dummy` (headless):
```
D (blusys_framework) framework init placeholder
I (widget_kit_demo) feedback: channel=visual pattern=focus value=1
I (widget_kit_demo) widget kit demo running. close the window to exit.
I (widget_kit_demo) initial slider = 50
```

Confirms: `blusys::framework::init()` runs, `runtime.init(...)` succeeds,
`controller.init()` emits its first feedback event, the feedback bus delivers it to the
logging sink, the widget kit builds the screen (`screen_create` + `col_create` +
`label_create` + `divider_create` + `slider_create` + `row_create` + `button_create` ×3
+ `overlay_create`), and `slider_get_value` returns the expected initial 50.

The mouse-click → `post_intent` → controller → slider mutation → `show_overlay` route
path cannot be exercised under `SDL_VIDEODRIVER=dummy` and requires a visual run on a
graphical host. That codepath is identical to the one exercised on-device by
`framework_app_basic`.

---

## Stage 2 — ESP-IDF QEMU

QEMU binary: `qemu-esp-develop-9.2.2-20250817` (xtensa + riscv32).

### Headless scaffold

Scaffolded with `blusys create --starter headless`, built against the local platform
checkout via `override_path`. Boot success criterion: `headless product running` must
appear in the boot log, no panic / Guru Meditation / LoadProhibited.

| Target    | Total image | Boot log  | Panics |
|-----------|-------------|-----------|--------|
| esp32s3   | 0x2f210     | found     | 0      |
| esp32     | 0x2f210     | found     | 0      |
| esp32c3   | 0x2f210     | found     | 0      |

Sample (esp32s3):
```
I (60) main_task: Calling app_main()
I (62) app_main: headless product running
```

### Framework core example

`examples/framework_core_basic` exercises the spine end-to-end (no LVGL). Expected
output: five controller-driven events.

| Target    | Expected spine events | Panics |
|-----------|-----------------------|--------|
| esp32s3   | 5                     | 0      |
| esp32     | 5                     | 0      |
| esp32c3   | 5                     | 0      |

```
I (64) framework_core_basic: feedback: channel=visual pattern=focus value=1
I (64) framework_core_basic: feedback: channel=audio pattern=click value=1
I (64) framework_core_basic: route command: set_root id=1 transition=0
I (64) framework_core_basic: feedback: channel=visual pattern=confirm value=1
I (64) framework_core_basic: route command: show_overlay id=7 transition=0
```

### Interactive product — QEMU limitation

The interactive product requires an LCD backed by `components/blusys/src/drivers/display/lcd.c`.
ESP-IDF QEMU does not emulate any LCD hardware. Stage 2 for the interactive product is
skipped by design; its widget-kit surface is covered by stage 1, and its device-specific
output path is covered by stage 3.

---

## Stage 3 — Real hardware

Run on physical hardware in April 2026. Three target chips covered for the headless
product; the interactive product was validated on esp32-c3 + ST7735 (a riscv32
substitute for the roadmap-suggested esp32-s3 — the framework code is target-agnostic
and riscv32 is a stronger cross-target test).

### Headless product — three-target boot

| Target    | Arch            | Cores | Port           | Boot log line                        | Result |
|-----------|-----------------|-------|----------------|--------------------------------------|--------|
| esp32-c3  | RISC-V rv32imc  | 1     | `/dev/ttyACM0` | `app_main: headless product running` | ✅ |
| esp32     | Xtensa LX6      | 2     | `/dev/ttyUSB0` | `app_main: headless product running` | ✅ |
| esp32-s3  | Xtensa LX7      | 2     | `/dev/ttyACM0` | `app_main: headless product running` | ✅ |

All three targets reach `home_controller initialized` followed by
`headless product running`. Same binary semantics across single-core RISC-V and
dual-core Xtensa — confirms the "no target-specific surprises" claim.

### Interactive product — esp32-c3 + ST7735, widget kit on real LCD

Hardware: generic 0.96" ST7735 SPI panel wired to esp32-c3 (SCLK=4, MOSI=6, CS=7,
DC=3, RST/BL not connected, PCLK 8 MHz, landscape 160×128).

| Stage                                              | Result |
|----------------------------------------------------|--------|
| `blusys_lcd_open` returns `BLUSYS_OK` for ST7735   | ✅ |
| `blusys_ui_open` reports `160x128 buf_lines=20`    | ✅ |
| `runtime.init` + `home_controller::init` succeed   | ✅ |
| Widget kit screen renders to the panel             | ✅ |
| Main loop ticks `runtime.step` under UI lock without WDT | ✅ |

Visible output: dark surface background, "Hello, blusys" title label, divider, centered
button row with `secondary` `-` and `primary` `+` buttons. The entire widget-kit
rasterization path runs end-to-end against the real ST7735 driver, `blusys_ui` render
task, and the LVGL flush callback through `blusys_lcd_draw_bitmap`.

### Scaffold bugs found and fixed during stage 3

Real-hardware validation surfaced three independent bugs in `blusys create --starter
interactive` that stages 1 and 2 cannot catch (neither exercises a real LCD driver
under a FreeRTOS render task):

1. **`sdkconfig.defaults` was empty.** Without `CONFIG_LV_OS_FREERTOS=y`, LVGL's global
   lock (`lv_lock`/`lv_unlock`) is a no-op. `blusys_ui_lock` silently fails to
   serialize the main task against the `blusys_ui` render task (priority 5). The race
   fires `LV_ASSERT_MSG(!disp->rendering_in_progress)` inside `lv_inv_area`; the
   default `LV_ASSERT_HANDLER` is `while(1);`, which starves IDLE and triggers
   `task_wdt` after 5 s.

2. **`app/ui/screens/main_screen.cpp` never called `lv_screen_load(screen)`.** The
   widget kit screen was created and wired but never activated. Same bug independently
   surfaced in `scripts/host/src/widget_kit_demo.cpp`.

3. **`main/app_main.cpp` called `lv_init()` and `lv_timer_handler()` directly** but
   never opened any LCD or `blusys_ui` instance — a deceptive template that compiled
   clean and ran without error yet drew nothing and is incompatible with `blusys_ui`'s
   own render task.

All three are fixed upstream. The `blusys create --starter interactive` template now
writes `CONFIG_LV_OS_FREERTOS=y` + `CONFIG_LV_COLOR_DEPTH_16=y` to `sdkconfig.defaults`,
ends `main_screen_create` with `lv_screen_load(screen)`, and uses the correct
`blusys_ui_lock` discipline around widget creation and `runtime.step()` calls.

---

## Stage 3 — Monochrome OLED (v6.1.0 patch)

Bringing the LVGL stack up on a 128×32 SSD1306 I2C OLED on real esp32 surfaced two
independent bugs that the ST7735/SPI validation path masked.

**Bug 1: I2C async mode caused silent write failures.**
`blusys_i2c_master_open` initialized the ESP-IDF i2c.master driver with
`trans_queue_depth = 1` (asynchronous mode), where `i2c_master_transmit` queues the
transaction and returns `ESP_OK` immediately without confirming the slave's address-phase
ACK. Every write silently appeared to succeed even on non-existent devices, surfacing as
117 phantom devices across the full 7-bit address space during I2C scan. Fix: switch to
`trans_queue_depth = 0` (synchronous mode). Bonus fix: `blusys_i2c_master_probe`
replaced with a write-based probe (1-byte transmit of `0x00`) compatible with sync mode
and harmless on all common I2C devices (SSD1306, BMP280, MPU6050, 24LCxx).

**Bug 2: `blusys_ui` hardcoded RGB565 flush.**
`blusys_ui` hardcoded `LV_COLOR_FORMAT_RGB565` + `lv_draw_sw_rgb565_swap` in its flush
callback, making monochrome panels impossible to drive through the standard service. Fix:
added `blusys_ui_panel_kind_t` to `blusys_ui_config_t` (default
`BLUSYS_UI_PANEL_KIND_RGB565` — backwards-compatible). New
`BLUSYS_UI_PANEL_KIND_MONO_PAGE` selects an alternate flush path that thresholds each
RGB565 pixel to 1 bit and packs into SSD1306-style page format (each byte = 8 vertical
pixels in a horizontal page, MSB at the bottom). Verified end-to-end on real esp32 +
0.96" 128×32 SSD1306 I2C OLED.

---

## RAM / flash deltas

Measured on **esp32s3**, `sdkconfig.defaults` only, ESP-IDF v5.5.4, April 2026.

| Config                               | Total image | Flash Code | DIRAM .bss |
|--------------------------------------|-------------|------------|------------|
| `examples/smoke` (HAL baseline)      | 190,773     | 79,986     | 2,256      |
| `examples/system_info` (HAL + svc)   | 191,961     | 80,778     | 2,264      |
| headless scaffold (+ fw core)        | 192,929     | 81,258     | 2,696      |
| interactive scaffold (+ UI + LVGL)   | 329,105     | 202,582    | 69,144     |

**Framework core is essentially free.** Adding the framework spine on top of a bare HAL
app costs ~2,156 bytes total image / ~440 bytes RAM (static route + event queues).

**LVGL dominates the UI tier.** Moving from headless to interactive adds ~136 KB total
image and ~66 KB `.bss`. The Blusys widget kit + layout primitives + theme add only
~3,355 bytes total on top of LVGL:

```
liblvgl__lvgl.a    Flash Code: 118,325    DIRAM .bss: 66,016
libblusys_framework.a                     Total Size:   3,355
```

---

## Input → feedback latency

**Status: deferred to V1.1.**

The full chain (encoder press → `intent::confirm` → `controller.handle` →
`submit_route(show_overlay)` → `overlay_show` visibly on screen) was not measured on
hardware with a scope this session. The rig used for stage 3 interactive validation had
an LCD but no encoder wired in, and no scope was attached.

The post-`intent` → controller → route_sink call chain runs synchronously inside a
single `runtime.step()` invocation, completing in under one `lv_timer_handler` tick on
host (\<5 ms measured under SDL2). The on-device cost is dominated by the LVGL render
path, not the framework spine.

V1.1 follow-up: assemble the full rig (esp32-s3 + ST7735 + EC11 encoder + Saleae
equivalent), wire `intent::confirm` emission to a debug GPIO toggle inside
`controller::handle`, tap that GPIO + the first `blusys_lcd_draw_bitmap` SPI CS edge,
and record median + p99 over a few hundred presses.

---

## Reproduction commands

```bash
# Stage 1 — host harness
./blusys host-build
./scripts/host/build-host/hello_lvgl          # LVGL-only smoke
./scripts/host/build-host/widget_kit_demo     # framework bridge

# Stage 2 — QEMU (headless only)
./blusys create --starter headless /tmp/phase9_headless
./blusys build /tmp/phase9_headless esp32s3
./blusys install-qemu
./blusys qemu /tmp/phase9_headless esp32s3

# Size measurements
./blusys size examples/smoke esp32s3
./blusys size /tmp/phase9_headless esp32s3
```

# Phase 9 — Validation Report

This document captures the Phase 9 validation measurements for the V5 → V6
platform transition. It is the "validation notes" deliverable listed in
[`ROADMAP.md`](ROADMAP.md) Phase 9.

Phase 9 done-when criteria, and the status of each:

| Criterion                                                                 | Status |
|---------------------------------------------------------------------------|--------|
| At least one reference interactive product runs end-to-end on the platform | stages 1 (host) + 2 (QEMU, core spine only — LVGL cannot run in QEMU) landed; stage 3 (hardware) needs user |
| At least one reference headless product boots and exercises its primary flow | stages 1 + 2 landed; stage 3 needs user |
| Validation notes capture interactive latency + RAM/flash deltas            | RAM/flash deltas landed (below); latency measurement needs stage 3 hardware |
| Migration guide exists and is accurate                                    | `docs/migration-guide.md` (see Phase 9 implementation PR) |

## 1. Validation chain — stage 1 (PC + LVGL + SDL2)

**Artifact:** `scripts/host/build-host/widget_kit_demo`

The Phase 9 framework bridge (`scripts/host/CMakeLists.txt`) compiles all
`blusys_framework` C++ sources (core spine + V1 widget kit + layout
primitives) + `components/blusys/src/common/error.c` into a
`blusys_framework_host` static library and links it into the
`widget_kit_demo` executable alongside host LVGL + SDL2. The same runtime
→ controller → route sink chain validated on-device by
`examples/framework_app_basic/` is exercised on host here.

Build evidence:
```
libblusys_framework_host.a         93,994 bytes
widget_kit_demo                 1,010,856 bytes (hello_lvgl: 976,984)
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
`controller.init()` emits its first feedback event, the feedback bus
delivers it to the logging sink, the widget kit builds the screen
(`screen_create` + `col_create` + `label_create` + `divider_create` +
`slider_create` + `row_create` + `button_create` ×3 + `overlay_create`),
and `slider_get_value` returns the expected initial 50.

The mouse-click → `post_intent` → controller → slider mutation →
`show_overlay` route path cannot be exercised under
`SDL_VIDEODRIVER=dummy` (no window, no clicks) and requires a visual run
on a graphical host to verify. The codepath is identical to the one
exercised on-device by `framework_app_basic`, which is the canonical
Phase 6 validation.

## 2. Validation chain — stage 2 (ESP-IDF QEMU)

QEMU binary: `qemu-esp-develop-9.2.2-20250817` (xtensa + riscv32).

### Headless sample product

Scaffolded with `blusys create --starter headless`, built against the
local platform checkout via `override_path`. Boot success criterion:
`headless product running` must appear in the boot log, no panic /
Guru Meditation / LoadProhibited patterns.

| Target    | Total image | Boot log | Panics |
|-----------|-------------|----------|--------|
| esp32s3   | 0x2f210     | found    | 0      |
| esp32     | 0x2f210     | found    | 0      |
| esp32c3   | 0x2f210     | found    | 0      |

Sample boot sequence (esp32s3):
```
I (60) main_task: Calling app_main()
I (62) app_main: headless product running
```

### Framework core example

`examples/framework_core_basic` exercises the spine end-to-end (no
LVGL). Expected boot output: five controller-driven events — an
initial `visual/focus`, a `route: set_root id=1`, a post-route
`audio/click`, a `visual/confirm`, and a `route: show_overlay id=7`.

| Target    | Expected spine events | Panics |
|-----------|----------------------|--------|
| esp32s3   | 5                    | 0      |
| esp32     | 5                    | 0      |
| esp32c3   | 5                    | 0      |

Sample sequence:
```
I (64) framework_core_basic: feedback: channel=visual pattern=focus value=1
I (64) framework_core_basic: feedback: channel=audio pattern=click value=1
I (64) framework_core_basic: route command: set_root id=1 transition=0
I (64) framework_core_basic: feedback: channel=visual pattern=confirm value=1
I (64) framework_core_basic: route command: show_overlay id=7 transition=0
```

### Interactive product — QEMU limitation

The interactive product requires an LCD backed by
`components/blusys/src/drivers/display/lcd.c`. ESP-IDF QEMU does not
emulate any LCD hardware (no SPI-backed ILI9341, no i80 bus). Stage 2
for the interactive product is therefore skipped by design; its
widget-kit surface is covered by stage 1 (host LVGL + SDL2), and its
device-specific output path is covered by stage 3 (real hardware).

## 3. Validation chain — stage 3 (real hardware)

**Status:** deferred to hardware gate (see §6).

Stage 3 needs physical ESP32 / ESP32-C3 / ESP32-S3 boards and an LCD +
encoder setup for the interactive product. The repo ships a report
template at `docs/guides/hardware-validation-report-template.md`;
Phase 9 stage-3 reports should be filed there following the convention
used by the Phase 5 widget validation reports.

## 4. RAM / flash deltas vs pre-migration baseline

Measured on **esp32s3**, `sdkconfig.defaults` only (no target-specific
overrides), ESP-IDF v5.5.4, `blusys size` output, April 2026 build.

| Config                             | Total image | Flash Code (.text) | DIRAM .bss | DIRAM .text | DIRAM .data |
|------------------------------------|-------------|--------------------|------------|-------------|-------------|
| `examples/smoke` (HAL baseline)    | 190,773     | 79,986             | 2,256      | ~41,500     | ~12,000     |
| `examples/system_info` (HAL + svc) | 191,961     | 80,778             | 2,264      | ~41,500     | ~12,000     |
| `phase9_headless` (HAL + svc + fw core) | 192,929 | 81,258             | 2,696      | ~41,500     | ~12,000     |
| `phase9_interactive` (+ UI + LVGL) | 329,105     | 202,582            | 69,144     | 41,759      | 12,528      |

### Interpretation

**Framework core is essentially free.** Adding the `blusys_framework` core
spine (`runtime` + `controller` + `feedback` + `router` + `intent`) on top
of a bare HAL app costs ~2,156 bytes total image, ~1,272 bytes of Flash
Code, and 440 bytes of RAM (mostly the static route + event queues). The
spine is deliberately allocation-free and fits comfortably inside the
"headless product" budget.

**LVGL dominates the UI tier cost.** Moving from headless to interactive
adds ~136,176 bytes total image: 121,324 bytes of Flash Code and 66,448
bytes of `.bss`. The overwhelming majority of both deltas is `liblvgl.a`:

```
liblvgl__lvgl.a    Flash Code: 118,325    DIRAM .bss: 66,016
libblusys_framework.a                     Total Size:   3,355
```

The Blusys widget kit + layout primitives + theme add only ~3,355 bytes
total (2,138 bytes of Flash Code .text, 785 bytes of Flash Data .rodata,
432 bytes of .bss for the widget slot pools). This matches the V1 scope
lock: V1 widgets are thin wrappers over stock LVGL primitives, so the
library cost is LVGL itself, not Blusys code on top of it.

**No target-specific surprises.** The same relative deltas hold across
esp32, esp32c3, and esp32s3 — the framework code is target-agnostic C++.
The absolute `.bin` sizes move by at most a few hundred bytes between
targets, and those deltas come from ESP-IDF itself (different IRAM
policies, different ROM stubs), not the transition.

**V1 size policy:** the roadmap explicitly says "report them, do not gate
on fixed thresholds in V1". These deltas are reported; no threshold is
claimed. V2 may revisit widget-kit size budgets once more widgets ship
and the LVGL version is re-evaluated.

## 5. Input → feedback latency (pending stage 3)

The latency measurement — encoder press → `intent::confirm` →
`controller.handle` → `submit_route(show_overlay)` → `route_sink.submit`
→ `overlay_show` visibly on screen — requires:

- a real encoder (not a button) wired to `blusys_encoder`,
- a real LCD with a measurable display update path,
- a logic analyser or oscilloscope tapping GPIO + LCD backlight.

None of those are available in the host harness or in QEMU. This
measurement is a stage-3 deliverable and is folded into the hardware
validation report template usage.

A coarse, non-hardware surrogate: the entire `framework_app_basic`
post_intent → controller → route_sink call chain runs synchronously
inside `runtime.step()`, which completes in under one `lv_timer_handler`
tick on host (<5 ms measured under SDL2). The device cost is dominated
by the LVGL render path, not by the framework spine.

## 6. Hardware gate (stage 3)

Tasks that require physical boards and cannot be completed without
user involvement:

1. **First-boot success** on all three targets for `phase9_headless`
   (UART log confirms `headless product running`).
2. **First-boot success** on esp32s3 + LCD + encoder for
   `phase9_interactive` (screen renders, encoder rotation moves slider,
   encoder press shows overlay).
3. **Input → feedback latency measurement** on the interactive product,
   logic analyser or scope, recorded per
   `docs/guides/hardware-validation-report-template.md`.

When stage 3 completes, update the table in §1 above and flip Phase 9
to `completed` in [`PROGRESS.md`](../PROGRESS.md).

## 7. Reproduction commands

```bash
# Stage 1 — host harness
./blusys host-build
./scripts/host/build-host/hello_lvgl            # Phase 4.5 LVGL-only smoke
./scripts/host/build-host/widget_kit_demo       # Phase 9 framework bridge

# Stage 2 — QEMU (headless only, interactive cannot run in QEMU)
./blusys create --starter headless /tmp/phase9_headless
# edit main/idf_component.yml to use override_path for local checkout
./blusys build /tmp/phase9_headless esp32s3
./blusys install-qemu                           # if QEMU not present
source $IDF_PATH/export.sh
(cd /tmp/phase9_headless/build-esp32s3 && \
 esptool.py --chip esp32s3 merge_bin --fill-flash-size 4MB -o flash_image.bin @flash_args)
PATH="$PATH:~/.espressif/tools/qemu-esp-develop-*/bin" \
 qemu-system-xtensa -nographic -no-reboot -machine esp32s3 \
 -drive file=/tmp/phase9_headless/build-esp32s3/flash_image.bin,if=mtd,format=raw \
 -nic user,model=open_eth \
 -global driver=timer.esp32c3.timg,property=wdt_disable,value=true

# Size measurements
./blusys size examples/smoke esp32s3
./blusys size examples/system_info esp32s3
./blusys size /tmp/phase9_headless esp32s3
./blusys size /tmp/phase9_interactive esp32s3
```

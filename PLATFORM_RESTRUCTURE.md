# Blusys Platform Restructure — v0 Architecture

This document captures the complete v0 architecture for the Blusys platform and
the phased implementation plan to get there. It is the authoritative design
reference for this restructure.

---

## Context

The platform currently ships as three separate ESP-IDF components:

- `components/blusys_hal/` — C HAL + drivers
- `components/blusys_services/` — C stateful runtime (Wi-Fi, storage, MQTT, …)
- `components/blusys_framework/` — C++ app framework

This split was historical. In practice:

- Products always require all three (transitive deps force it)
- Services are 95% ESP-IDF wrappers — as device-coupled as HAL
- The `framework/` internal layout (`app/`, `core/`, `ui/`, `integration/`) does
  not mirror the product scaffold cleanly
- Several singleton directories exist (`drivers/actuator/`, `drivers/sensor/`,
  `widgets/button/button.hpp`, `ui/input/`, `ui/icons/`, …)
- Panel-specific knowledge (ILI9341 is 240×320) lives in `framework/`,
  violating the "device-agnostic framework" principle

**Goal:** a single coherent platform, two clean layers, minimal ceremony,
architecturally honest, lint-enforced, and structured so it stays
maintainable as the codebase grows.

---

## Design principles

These are the non-negotiable invariants of the v0 architecture.

### 1. One merged component

All platform code lives in a single ESP-IDF component: `components/blusys/`.
Products depend on it with `REQUIRES blusys`. Internal layering is a
directory-and-lint concern, not a component-boundary concern.

### 2. Two build layers

- **`hal/`** — device-coupled C code (everything tied to ESP-IDF APIs)
- **`framework/`** — device-agnostic C++ app runtime

Services collapse into `hal/services/` because they are fundamentally
ESP-IDF-coupled. The tiny amount of truly portable logic scattered inside
them (HID parsing, pixel conversion) is not worth a third layer.

### 3. `src/` mirrors `include/blusys/` exactly

For every public header at `include/blusys/X/Y/Z.h`, the corresponding source
lives at `src/X/Y/Z.c` (or `.cpp`). No exceptions. Enforceable by lint.

### 4. One-way layering

- `hal/**` must NOT include `blusys/framework/*`
- `framework/{app,core,ui}/**` must NOT include `blusys/hal/*`
- Only `framework/platform/**` may bridge the two

The HAL public API IS the contract. Framework pure zones program against it
through `framework/platform/`. This is lint-enforced from day one.

### 5. `platform/` is the escape-hatch zone (both in framework AND product)

Framework's `framework/platform/` is the only place framework code may
include HAL headers. Product's `main/platform/` is the only place product
code *should* (by convention) include HAL headers.

Same word, same role, mirrored at two levels.

### 6. No singleton directories

Flat wins over sub-dirs that contain 1-2 files. Widgets, drivers,
primitives — all flat. Sub-dirs only when a group has a real common category
and enough files to justify the ceremony.

### 7. Layered umbrella headers

Instead of one giant umbrella:

- `blusys/hal.h` — HAL only (C)
- `blusys/framework.hpp` — framework only (C++)
- `blusys/blusys.hpp` — full platform (convenience)

Products that only need HAL pay zero framework compile cost.

### 8. Private headers stay local

A header included by exactly one `.cpp` lives next to that `.cpp` under
`src/`, not in `include/**/internal/`. Only cross-file internals go in
`internal/` dirs.

### 9. Framework is truly device-agnostic

No panel dimensions hardcoded under `framework/`. Panel hardware data
(width, height, protocol defaults) lives in `hal/drivers/panels/`. Framework
composes via panel defaults exposed from HAL.

### 10. Product scaffold mirrors the framework

```
Product main/           Framework layers:
  core/          ↔       framework/core/
  ui/            ↔       framework/ui/
  platform/      ↔       framework/platform/
```

Plus `framework/app/` for the API contract (no product-side equivalent).
Same word, same meaning, consistent mental model at every level.

---

## Final architecture

### Top-level layout

```
components/blusys/
├── CMakeLists.txt
├── Kconfig
├── README.md
├── idf_component.yml
├── include/blusys/
│   ├── blusys.h              ← C-level full umbrella (HAL only, for C code)
│   ├── blusys.hpp            ← C++-level full umbrella (HAL + framework)
│   ├── hal.h                 ← HAL-only umbrella (C)
│   ├── framework.hpp         ← framework-only umbrella (C++)
│   ├── hal/
│   │   └── (see below)
│   └── framework/
│       └── (see below)
└── src/
    ├── hal/
    │   └── (mirrors include/blusys/hal/)
    └── framework/
        └── (mirrors include/blusys/framework/)
```

### HAL layer

```
include/blusys/hal/
├── peripheral/                ← silicon wrappers (24 files flat, all C)
│   ├── adc.h  dac.h  efuse.h  gpio.h  gpio_expander.h
│   ├── i2c.h  i2c_slave.h  i2s.h  mcpwm.h  nvs.h
│   ├── one_wire.h  pcnt.h  pwm.h  rmt.h  sd_spi.h
│   ├── sdm.h  sdmmc.h  sleep.h  spi.h  spi_slave.h
│   ├── system.h  target.h  temp_sensor.h  timer.h
│   ├── touch.h  twai.h  uart.h  ulp.h  usb_device.h
│   ├── usb_host.h  version.h  wdt.h  error.h
│
├── drivers/                   ← composite hardware drivers (flat, C)
│   ├── button.h  buzzer.h  dht.h  encoder.h
│   ├── lcd.h  led_strip.h  seven_seg.h
│   └── panels/                ← panel HAL defaults (NEW: moved from framework)
│       ├── ili9341.h  ili9488.h  st7735.h  st7789.h
│       └── ssd1306.h  qemu_rgb.h
│
├── services/                  ← stateful runtime modules (C)
│   ├── connectivity/
│   │   ├── wifi.h  wifi_prov.h  wifi_mesh.h
│   │   ├── bluetooth.h  ble_gatt.h  espnow.h
│   │   └── protocol/
│   │       ├── http_client.h  http_server.h  mqtt.h
│   │       ├── ws_client.h  mdns.h
│   │       └── sntp.h  local_ctrl.h
│   ├── storage/
│   │   ├── fatfs.h  fs.h
│   ├── system/                ← renamed from device/
│   │   ├── console.h  ota.h  power_mgmt.h
│   ├── input/
│   │   └── usb_hid.h
│   └── output/
│       └── display.h
│
├── internal/                  ← cross-file HAL internals
│   ├── blusys_bt_stack.h  blusys_esp_err.h
│   ├── blusys_global_lock.h  blusys_internal.h
│   ├── blusys_lock.h  blusys_nvs_init.h
│   ├── blusys_target_caps.h  blusys_timeout.h
│   └── lcd_panel_ili.h
│
└── targets/                   ← per-MCU capability tables
    ├── esp32/
    ├── esp32c3/
    └── esp32s3/
```

### Framework layer

```
include/blusys/framework/
├── app/                       ← the API contract
│   ├── app.hpp  app_ctx.hpp  app_spec.hpp
│   ├── app_services.hpp  app_identity.hpp
│   ├── entry.hpp
│   ├── integration_dispatch.hpp  variant_dispatch.hpp
│   ├── theme_presets.hpp
│   ├── capabilities/          ← capability system (everything consolidated here)
│   │   ├── capability.hpp     ← base type (was flat in app/)
│   │   ├── event.hpp          ← was flat in app/ as capability_event.hpp
│   │   ├── list.hpp           ← was flat in app/ as capability_list.hpp
│   │   ├── capabilities.hpp   ← umbrella
│   │   ├── bluetooth.hpp  connectivity.hpp  diagnostics.hpp
│   │   ├── lan_control.hpp  mqtt_host.hpp  ota.hpp
│   │   ├── provisioning.hpp  storage.hpp
│   │   └── telemetry.hpp  usb.hpp
│   ├── flows/                 ← lifecycle orchestration
│   │   ├── flows.hpp
│   │   ├── boot.hpp  loading.hpp  error.hpp
│   │   ├── status.hpp  settings.hpp
│   │   ├── connectivity_flow.hpp  provisioning_flow.hpp
│   │   ├── diagnostics_flow.hpp  ota_flow.hpp
│   └── internal/              ← framework-internal (was detail/)
│       ├── app_runtime.hpp  default_route_sink.hpp
│       └── feedback_logging_sink.hpp  pending_events.hpp
│
├── core/                      ← runtime engine (PURE — no HAL includes)
│   ├── runtime.hpp  intent.hpp  router.hpp
│   ├── feedback.hpp  feedback_presets.hpp
│   └── containers.hpp
│
├── ui/                        ← UI toolkit (PURE — no HAL includes)
│   ├── theme.hpp  fonts.hpp  callbacks.hpp
│   ├── transition.hpp  visual_feedback.hpp
│   ├── encoder.hpp  focus_scope.hpp           ← flat (was ui/input/)
│   ├── icon_set.hpp  icon_set_minimal.hpp     ← flat (was ui/icons/)
│   ├── view.hpp  page.hpp  shell.hpp  overlay_manager.hpp  ← moved from app/view/
│   ├── screen_registry.hpp  screen_router.hpp
│   ├── bindings.hpp  action_widgets.hpp  composites.hpp
│   ├── custom_widget.hpp  lvgl_scope.hpp
│   ├── primitives/
│   │   ├── col.hpp  row.hpp  screen.hpp  label.hpp
│   │   └── divider.hpp  icon_label.hpp  key_value.hpp  status_badge.hpp
│   ├── widgets/               ← FLAT (was 17 singleton sub-dirs)
│   │   ├── button.hpp  card.hpp  chart.hpp  data_table.hpp
│   │   ├── dropdown.hpp  gauge.hpp  input_field.hpp  knob.hpp
│   │   ├── level_bar.hpp  list.hpp  modal.hpp  overlay.hpp
│   │   └── progress.hpp  slider.hpp  tabs.hpp  toggle.hpp  vu_strip.hpp
│   └── screens/               ← moved from app/screens/
│       ├── about.hpp  diagnostics.hpp  status.hpp
│
└── platform/                  ← framework ↔ HAL bridge (ONLY zone allowed to #include hal/*)
    ├── profile.hpp            ← was platform_profile.hpp
    ├── host.hpp               ← was host_profile.hpp
    ├── headless.hpp           ← was profiles/headless.hpp
    ├── auto.hpp               ← was auto_profile.hpp
    ├── layout_surface.hpp
    ├── auto_shell.hpp
    ├── build.hpp              ← was build_profile.hpp
    ├── reference_build.hpp    ← was reference_build_profile.hpp
    ├── button_array_bridge.hpp
    ├── input_bridge.hpp
    ├── touch_bridge.hpp
    └── profiles/              ← OPTIONAL convenience bundles (can grow)
        ├── ili9341_with_encoder.hpp
        ├── ssd1306_i2c.hpp
        └── ... (future)
```

### Umbrella headers

- **`include/blusys/hal.h`** — C-only. Includes all of `blusys/hal/peripheral/`,
  `blusys/hal/drivers/`, `blusys/hal/services/`. Consumers who only need HAL
  use this.

- **`include/blusys/framework.hpp`** — C++-only. Includes framework public API
  (app, core, ui, platform). No HAL transitively.

- **`include/blusys/blusys.h`** — C alias for `hal.h` (for compatibility with
  product code that only uses HAL from C).

- **`include/blusys/blusys.hpp`** — Full platform umbrella (HAL + framework).
  Convenience for product C++ code.

### Product scaffold

Unchanged in concept, renamed directory:

```
your_product/main/
├── core/         ← state + actions + reducer  (PURE: framework only)
├── ui/           ← screens                    (PURE: framework only)
├── platform/     ← hardware + app_main        (escape hatch: may include hal/)
├── CMakeLists.txt
└── idf_component.yml
```

The `main/integration/` → `main/platform/` rename aligns with the framework's
platform naming. Everything follows the same vocabulary.

---

## Lint-enforced invariants

Updated `scripts/lint-layering.sh` enforces:

### Rule 1 — HAL cannot depend on framework

```bash
grep -rEn --include='*.c' --include='*.h' \
    '#include[[:space:]]*[<"]blusys/framework/' \
    "$repo_root/components/blusys/src/hal" \
    "$repo_root/components/blusys/include/blusys/hal" \
    || true
```

Any hit fails CI.

### Rule 2 — Pure framework cannot depend on HAL

```bash
grep -rEn --include='*.hpp' --include='*.cpp' \
    --exclude-dir=platform \
    '#include[[:space:]]*[<"]blusys/hal/' \
    "$repo_root/components/blusys/src/framework" \
    "$repo_root/components/blusys/include/blusys/framework" \
    || true
```

`framework/app/`, `framework/core/`, `framework/ui/` must be clean.
`framework/platform/` is the allowed exception.

### Rule 3 — Drivers cannot include peripherals directly from internal layer

Existing driver-internal rule carried over.

### Rule 4 — `src/` must mirror `include/blusys/`

For every `src/X/Y/Z.cpp`, there must be a corresponding
`include/blusys/X/Y/Z.hpp` (or `Z.h`), and vice versa, except:
- Private headers colocated in `src/`
- Internal-only sources not exposed publicly

A Python script verifies this in CI.

---

## Implementation plan

The restructure touches the entire codebase. It is broken into **8 phases**,
each landing as a single atomic commit. Phases must land in order.

### Phase 0 — Preparation

**Goal:** baseline snapshot and tooling.

- Capture current file inventory: `find components/ -type f | sort > /tmp/inventory-before.txt`
- Identify all consumers of current paths (examples, docs, scripts, cmake)
- Dry-run the path migration script to validate all paths are accounted for

Output: confirmed migration list, no actions taken.

### Phase 1 — Create the merged component shell

**Goal:** stand up `components/blusys/` with build skeleton.

Actions:
- Create `components/blusys/` directory
- Create minimal `CMakeLists.txt` (empty SRCS list initially, will be populated in phase 4)
- Create `Kconfig` merging the three existing Kconfigs
- Create `idf_component.yml` merging existing ones
- Create empty `include/blusys/` and `src/` directories

Deliverable: new component exists but compiles nothing.

### Phase 2 — Move HAL under `blusys/hal/`

**Goal:** relocate HAL sources + headers under the new layout, including panel
defaults extraction.

Git moves (preserve history):

```
components/blusys_hal/include/blusys/*.h              → components/blusys/include/blusys/hal/peripheral/*.h
components/blusys_hal/include/blusys/drivers/*.h      → components/blusys/include/blusys/hal/drivers/*.h
components/blusys_hal/include/blusys/internal/*.h     → components/blusys/include/blusys/hal/internal/*.h
components/blusys_hal/src/*.c                         → components/blusys/src/hal/peripheral/*.c
components/blusys_hal/src/drivers/*.c                 → components/blusys/src/hal/drivers/*.c
components/blusys_hal/src/targets/                    → components/blusys/src/hal/targets/
components/blusys_hal/src/ulp/                        → components/blusys/src/hal/ulp/
```

Then **NEW FILES** (panel defaults extracted from framework):

```
components/blusys/include/blusys/hal/drivers/panels/ili9341.h
components/blusys/include/blusys/hal/drivers/panels/ili9488.h
components/blusys/include/blusys/hal/drivers/panels/st7735.h
components/blusys/include/blusys/hal/drivers/panels/st7789.h
components/blusys/include/blusys/hal/drivers/panels/ssd1306.h
components/blusys/include/blusys/hal/drivers/panels/qemu_rgb.h
```

Each contains panel HAL defaults only — dimensions, driver enum, protocol
config. No framework types.

Intra-HAL include paths update:
- Every `#include "blusys/gpio.h"` → `#include "blusys/hal/peripheral/gpio.h"`
- Every `#include "blusys/drivers/lcd.h"` → `#include "blusys/hal/drivers/lcd.h"`
- Every `#include "blusys/internal/..."` → `#include "blusys/hal/internal/..."`

Also create:
- `include/blusys/hal.h` — HAL umbrella

Remove: `components/blusys_hal/` entirely.

### Phase 3 — Move Services under `blusys/hal/services/`

**Goal:** fold services into HAL (device-coupled layer).

Git moves:

```
components/blusys_services/include/blusys/connectivity/       → components/blusys/include/blusys/hal/services/connectivity/
components/blusys_services/include/blusys/storage/            → components/blusys/include/blusys/hal/services/storage/
components/blusys_services/include/blusys/device/             → components/blusys/include/blusys/hal/services/system/
components/blusys_services/include/blusys/input/              → components/blusys/include/blusys/hal/services/input/
components/blusys_services/include/blusys/output/             → components/blusys/include/blusys/hal/services/output/
components/blusys_services/src/connectivity/                  → components/blusys/src/hal/services/connectivity/
components/blusys_services/src/storage/                       → components/blusys/src/hal/services/storage/
components/blusys_services/src/device/                        → components/blusys/src/hal/services/system/
components/blusys_services/src/input/                         → components/blusys/src/hal/services/input/
components/blusys_services/src/output/                        → components/blusys/src/hal/services/output/
```

Note the rename: `device/` → `system/` (the old category name was confusing;
the files are `console`, `ota`, `power_mgmt` — all system lifecycle).

Intra-services include updates:
- `#include "blusys/connectivity/wifi.h"` → `#include "blusys/hal/services/connectivity/wifi.h"`
- `#include "blusys/output/display.h"` → `#include "blusys/hal/services/output/display.h"`
- `#include "blusys/device/ota.h"` → `#include "blusys/hal/services/system/ota.h"`
- etc.

Update `hal.h` umbrella to include services.

Remove: `components/blusys_services/` entirely.

### Phase 4 — Move Framework under `blusys/framework/`

**Goal:** relocate framework with full internal restructure.

This is the largest phase. Includes multiple structural changes simultaneously
because they're all coupled:

**4a — Path relocations:**
```
components/blusys_framework/include/blusys/app/              → components/blusys/include/blusys/framework/app/
components/blusys_framework/include/blusys/framework/core/   → components/blusys/include/blusys/framework/core/
components/blusys_framework/include/blusys/framework/ui/     → components/blusys/include/blusys/framework/ui/
components/blusys_framework/src/app/                         → components/blusys/src/framework/app/
components/blusys_framework/src/core/                        → components/blusys/src/framework/core/
components/blusys_framework/src/ui/                          → components/blusys/src/framework/ui/
```

**4b — Rename `integration/` → `platform/`:**
```
framework/app/platform_profile.hpp         → framework/platform/profile.hpp
framework/app/host_profile.hpp             → framework/platform/host.hpp
framework/app/auto_profile.hpp             → framework/platform/auto.hpp
framework/app/build_profile.hpp            → framework/platform/build.hpp
framework/app/reference_build_profile.hpp  → framework/platform/reference_build.hpp
framework/app/layout_surface.hpp           → framework/platform/layout_surface.hpp
framework/app/auto_shell.hpp               → framework/platform/auto_shell.hpp
framework/app/button_array_bridge.hpp      → framework/platform/button_array_bridge.hpp
framework/app/input_bridge.hpp             → framework/platform/input_bridge.hpp
framework/app/touch_bridge.hpp             → framework/platform/touch_bridge.hpp
framework/app/profiles/headless.hpp        → framework/platform/headless.hpp
framework/app/profiles/host.hpp            ← merged into framework/platform/host.hpp
framework/app/profiles/ili9341.hpp         → framework/platform/profiles/ili9341.hpp
framework/app/profiles/ili9488.hpp         → framework/platform/profiles/ili9488.hpp
framework/app/profiles/st7735.hpp          → framework/platform/profiles/st7735.hpp
framework/app/profiles/st7789.hpp          → framework/platform/profiles/st7789.hpp
framework/app/profiles/ssd1306.hpp         → framework/platform/profiles/ssd1306.hpp
framework/app/profiles/qemu_rgb.hpp        → framework/platform/profiles/qemu_rgb.hpp
```

Panel profile .hpp files are REWRITTEN to use `blusys/hal/drivers/panels/*.h`
defaults rather than hardcoding dimensions.

**4c — Move view files from `app/view/` to `ui/`:**
```
framework/app/view/view.hpp              → framework/ui/view.hpp
framework/app/view/page.hpp              → framework/ui/page.hpp
framework/app/view/shell.hpp             → framework/ui/shell.hpp
framework/app/view/overlay_manager.hpp   → framework/ui/overlay_manager.hpp
framework/app/view/screen_registry.hpp   → framework/ui/screen_registry.hpp
framework/app/view/screen_router.hpp     → framework/ui/screen_router.hpp
framework/app/view/bindings.hpp          → framework/ui/bindings.hpp
framework/app/view/action_widgets.hpp    → framework/ui/action_widgets.hpp
framework/app/view/composites.hpp        → framework/ui/composites.hpp
framework/app/view/custom_widget.hpp     → framework/ui/custom_widget.hpp
framework/app/view/lvgl_scope.hpp        → framework/ui/lvgl_scope.hpp
```

Same for corresponding `src/` files (which were all flat in `src/app/`
historically, so they move to `src/framework/ui/`).

**4d — Move screens to UI layer:**
```
framework/app/screens/about_screen.hpp       → framework/ui/screens/about.hpp
framework/app/screens/diagnostics_screen.hpp → framework/ui/screens/diagnostics.hpp
framework/app/screens/status_screen.hpp      → framework/ui/screens/status.hpp
framework/app/screens/screens.hpp            → framework/ui/screens/screens.hpp (umbrella)
```

Drop the `_screen` suffix (redundant inside `screens/`).

**4e — Consolidate capabilities:**
```
framework/app/capability.hpp           → framework/app/capabilities/capability.hpp
framework/app/capability_event.hpp     → framework/app/capabilities/event.hpp
framework/app/capability_list.hpp      → framework/app/capabilities/list.hpp
framework/app/capabilities_inline.hpp  → framework/app/capabilities/inline.hpp
```

All capability-related files live inside `capabilities/`. No more floaters.

**4f — Rename `detail/` to `internal/`:**
```
framework/app/detail/ → framework/app/internal/
```

**4g — Flatten UI sub-directories:**

Widget singleton dirs:
```
framework/ui/widgets/button/button.hpp         → framework/ui/widgets/button.hpp
framework/ui/widgets/card/card.hpp             → framework/ui/widgets/card.hpp
framework/ui/widgets/chart/chart.hpp           → framework/ui/widgets/chart.hpp
... (17 widgets)
```

UI input/icon dirs:
```
framework/ui/input/encoder.hpp        → framework/ui/encoder.hpp
framework/ui/input/focus_scope.hpp    → framework/ui/focus_scope.hpp
framework/ui/icons/icon_set.hpp       → framework/ui/icon_set.hpp
framework/ui/icons/icon_set_minimal.hpp → framework/ui/icon_set_minimal.hpp
```

Same for corresponding `src/framework/ui/widgets/*/*.cpp` → `src/framework/ui/widgets/*.cpp`.

**4h — Drop `ui/primitives.hpp` + `ui/widgets.hpp` umbrellas or update them:**

Review and update umbrella headers to reflect new paths.

**4i — Intra-framework include path updates:**

Every `#include "blusys/app/..."` → `#include "blusys/framework/app/..."`
Every `#include "blusys/framework/core/..."` → `#include "blusys/framework/core/..."` (path stays the same, just relocated)
Every `#include "blusys/framework/ui/..."` → `#include "blusys/framework/ui/..."`

Plus all the internal moves from 4b-4g.

**4j — Create `framework.hpp` umbrella.**

Remove: `components/blusys_framework/` entirely.

### Phase 5 — Build system

**Goal:** single merged CMakeLists.txt.

Actions:
- Write `components/blusys/CMakeLists.txt` with all sources listed by layer
- Merge dependencies from old three CMakeLists
- Handle conditional per-target code (targets/, ULP)
- Handle optional managed components (tinyusb, esp_lcd_qemu_rgb)
- Handle LVGL conditional compilation
- Update `cmake/blusys_host_bridge.cmake` paths
- Update `cmake/blusys_framework_ui_sources.cmake` or remove

### Phase 6 — Consumer update sweep

**Goal:** update every reference outside `components/blusys/`.

Files to audit and update:
- All `examples/**/*.{c,cpp,cmake}` include paths
- All `examples/**/main/CMakeLists.txt` (REQUIRES changes from `blusys_hal blusys_services blusys_framework` → `blusys`)
- All `examples/**/main/idf_component.yml`
- All `docs/**/*.md` references to old paths
- All `scripts/*.sh` and `scripts/*.py` that reference component paths
- `inventory.yml` if it references old paths
- `README.md` architecture diagram
- `CLAUDE.md` or other top-level instructions

Master search list:
```bash
rg 'blusys_hal'
rg 'blusys_services'
rg 'blusys_framework'
rg 'blusys/app/'                # old framework app path
rg 'blusys/framework/core'      # verify still correct (just relocated)
rg 'blusys/drivers/'            # old HAL drivers path
rg 'blusys/connectivity/'       # old services path
rg 'blusys/storage/'            # old services path
rg 'blusys/device/'             # old services path (now system)
rg 'blusys/input/'              # old services path
rg 'blusys/output/'             # old services path
rg 'blusys/internal/'           # old HAL internal path
```

Every hit gets updated in Phase 6.

### Phase 7 — Lint and CI updates

**Goal:** machine-enforce the new invariants.

Actions:
- Rewrite `scripts/lint-layering.sh` with Rules 1-3 above
- Add new lint check: `src/` vs `include/blusys/` mirror validator (Python script)
- Update `scripts/check-host-bridge-spine.py` with new `blusys_host_bridge.cmake` path
- Add new CI job: `blusys-layer-invariants` that runs all layering checks
- Verify existing CI workflows still work against merged component

### Phase 8 — Product scaffold update

**Goal:** update `blusys create` scaffolding and docs.

Actions:
- Rename `main/integration/` → `main/platform/` in scaffold templates
- Update scaffold-emitted include paths
- Update `blusys create --list` descriptions if needed
- Update `docs/start/` and `docs/app/` to reflect new paths
- Update `docs/internals/architecture.md` with new diagram
- Update `docs/hal/` to reflect unified layer
- Update `docs/services/` — services are now `hal/services/`
- Bump version in `components/blusys/include/blusys/hal/peripheral/version.h` to 7.0.0
  (breaking change — major version bump)

---

## Migration reference tables

### HAL peripherals (flat)

All `include/blusys/<name>.h` → `include/blusys/hal/peripheral/<name>.h`.
All `src/<name>.c` → `src/hal/peripheral/<name>.c`.

### HAL drivers

All `include/blusys/drivers/<name>.h` → `include/blusys/hal/drivers/<name>.h`.
All `src/drivers/<name>.c` → `src/hal/drivers/<name>.c`.

**New directory:** `include/blusys/hal/drivers/panels/` (NEW panel hardware defaults).

### HAL services

| Old | New |
|---|---|
| `blusys/connectivity/wifi.h` | `blusys/hal/services/connectivity/wifi.h` |
| `blusys/connectivity/protocol/mqtt.h` | `blusys/hal/services/connectivity/protocol/mqtt.h` |
| `blusys/storage/fatfs.h` | `blusys/hal/services/storage/fatfs.h` |
| `blusys/device/console.h` | `blusys/hal/services/system/console.h` |
| `blusys/device/ota.h` | `blusys/hal/services/system/ota.h` |
| `blusys/device/power_mgmt.h` | `blusys/hal/services/system/power_mgmt.h` |
| `blusys/input/usb_hid.h` | `blusys/hal/services/input/usb_hid.h` |
| `blusys/output/display.h` | `blusys/hal/services/output/display.h` |

### Framework app

| Old | New |
|---|---|
| `blusys/app/app.hpp` | `blusys/framework/app/app.hpp` |
| `blusys/app/app_ctx.hpp` | `blusys/framework/app/app_ctx.hpp` |
| `blusys/app/capability.hpp` | `blusys/framework/app/capabilities/capability.hpp` |
| `blusys/app/capability_event.hpp` | `blusys/framework/app/capabilities/event.hpp` |
| `blusys/app/capability_list.hpp` | `blusys/framework/app/capabilities/list.hpp` |
| `blusys/app/capabilities_inline.hpp` | `blusys/framework/app/capabilities/inline.hpp` |
| `blusys/app/capabilities/connectivity.hpp` | `blusys/framework/app/capabilities/connectivity.hpp` |
| `blusys/app/flows/boot.hpp` | `blusys/framework/app/flows/boot.hpp` |
| `blusys/app/detail/app_runtime.hpp` | `blusys/framework/app/internal/app_runtime.hpp` |

### Framework core

| Old | New |
|---|---|
| `blusys/framework/core/runtime.hpp` | `blusys/framework/core/runtime.hpp` (path identical, relocated) |
| (all other core files same pattern) | |

### Framework ui

| Old | New |
|---|---|
| `blusys/framework/ui/widgets/button/button.hpp` | `blusys/framework/ui/widgets/button.hpp` |
| `blusys/framework/ui/input/encoder.hpp` | `blusys/framework/ui/encoder.hpp` |
| `blusys/framework/ui/icons/icon_set.hpp` | `blusys/framework/ui/icon_set.hpp` |
| `blusys/app/view/page.hpp` | `blusys/framework/ui/page.hpp` |
| `blusys/app/view/shell.hpp` | `blusys/framework/ui/shell.hpp` |
| `blusys/app/view/*.hpp` (11 files) | `blusys/framework/ui/*.hpp` |
| `blusys/app/screens/about_screen.hpp` | `blusys/framework/ui/screens/about.hpp` |
| `blusys/app/screens/diagnostics_screen.hpp` | `blusys/framework/ui/screens/diagnostics.hpp` |
| `blusys/app/screens/status_screen.hpp` | `blusys/framework/ui/screens/status.hpp` |

### Framework platform

| Old | New |
|---|---|
| `blusys/app/platform_profile.hpp` | `blusys/framework/platform/profile.hpp` |
| `blusys/app/host_profile.hpp` | `blusys/framework/platform/host.hpp` |
| `blusys/app/auto_profile.hpp` | `blusys/framework/platform/auto.hpp` |
| `blusys/app/build_profile.hpp` | `blusys/framework/platform/build.hpp` |
| `blusys/app/reference_build_profile.hpp` | `blusys/framework/platform/reference_build.hpp` |
| `blusys/app/layout_surface.hpp` | `blusys/framework/platform/layout_surface.hpp` |
| `blusys/app/auto_shell.hpp` | `blusys/framework/platform/auto_shell.hpp` |
| `blusys/app/button_array_bridge.hpp` | `blusys/framework/platform/button_array_bridge.hpp` |
| `blusys/app/input_bridge.hpp` | `blusys/framework/platform/input_bridge.hpp` |
| `blusys/app/touch_bridge.hpp` | `blusys/framework/platform/touch_bridge.hpp` |
| `blusys/app/profiles/ili9341.hpp` | `blusys/framework/platform/profiles/ili9341.hpp` |
| `blusys/app/profiles/headless.hpp` | `blusys/framework/platform/headless.hpp` |

### Product scaffold

| Old | New |
|---|---|
| `main/integration/` | `main/platform/` |
| `main/integration/app_main.cpp` | `main/platform/app_main.cpp` |

---

## Verification strategy

After each phase, run:

```bash
# 1. Layer invariants
bash scripts/lint-layering.sh

# 2. Src/include mirror check
python3 scripts/check-src-include-mirror.py

# 3. Stale-path audit
rg 'blusys_hal' --glob '*.{c,h,hpp,cpp,cmake,txt,md,yml}'
rg 'blusys_services' --glob '*.{c,h,hpp,cpp,cmake,txt,md,yml}'
rg 'blusys_framework' --glob '*.{c,h,hpp,cpp,cmake,txt,md,yml}'
rg 'blusys/app/view/' --glob '*.{c,h,hpp,cpp}'
rg 'blusys/app/capability\.' --glob '*.{c,h,hpp,cpp}'
rg 'blusys/app/platform_profile' --glob '*.{c,h,hpp,cpp}'
rg 'blusys/connectivity/' --glob '*.{c,h,hpp,cpp}'
rg 'blusys/drivers/input/' --glob '*.{c,h,hpp,cpp}'
rg 'blusys/drivers/display/' --glob '*.{c,h,hpp,cpp}'
rg 'widgets/[a-z_]*/[a-z_]*\.hpp' --glob '*.{c,h,hpp,cpp}'
rg 'framework/ui/input/' --glob '*.{c,h,hpp,cpp}'
rg 'framework/ui/icons/' --glob '*.{c,h,hpp,cpp}'

# 4. Full builds (representative examples)
blusys build examples/quickstart/hello_blusys esp32s3
blusys build examples/quickstart/hello_blusys esp32c3
blusys build examples/quickstart/hello_blusys esp32
blusys build examples/reference/interactive_dashboard esp32s3
blusys host-build examples/quickstart/hello_blusys

# 5. Host-bridge spine
python3 scripts/check-host-bridge-spine.py

# 6. Scaffold sanity
blusys create --dry-run my_test_product
```

All must pass before committing each phase.

---

## Non-goals / explicit deferrals

The following are **intentionally not done** in this restructure:

- **Pulling "device-agnostic business logic" out of services.** The 4 files
  with portable code (`ble_gatt.c`, `local_ctrl.c`, `usb_hid.c`, `display.c`)
  are 95% ESP-IDF-glued. Extracting them is not worth the complexity for v0.

- **Test directory structure.** When tests are added, they go in
  `components/blusys/tests/` mirroring `src/`. No convention is codified
  until the first test lands.

- **Splitting `services/` by "input/output".** The current 5 sub-categories
  cover all files. If a future service doesn't fit (audio, crypto), add a
  top-level service category then. No imagined slots.

- **Extension mechanism for external capabilities.** Products can already
  create capabilities in `main/` or in private components. No framework
  change needed.

- **C++ concepts / traits / virtual interfaces for HAL abstraction.** The
  HAL's C API surface IS the interface. Adding a C++ trait layer gains
  nothing and costs clarity.

---

## Risks and mitigations

| Risk | Mitigation |
|---|---|
| Massive single commit for Phase 4 is hard to review | Phase 4 is one atomic commit because intermediate states don't build. Document the structural changes clearly in the commit message. |
| Examples and docs drift while restructure is in progress | Phase 6 is a single sweep; nothing lands until all consumers are green. |
| Breaking change for downstream teams | v0 → v1.0 means breaking changes are acceptable; document migration in CHANGELOG and README. |
| Lost git history on renames | Use `git mv` for every rename. `git log --follow` still works. |
| Panel profile rewrite breaks product assumptions | Panel .hpp files in `framework/platform/profiles/` produce the same `device_profile` output; only the internals change. Product API unchanged. |
| Someone depends on old include paths via non-tracked code (private forks) | Out of scope. v0 breaking change is the contract. |

---

## Versioning

Current version: **6.1.1**.

After this restructure: **7.0.0** (major version bump for breaking API paths).

Update `components/blusys/include/blusys/hal/peripheral/version.h`:

```c
#define BLUSYS_VERSION_MAJOR  7
#define BLUSYS_VERSION_MINOR  0
#define BLUSYS_VERSION_PATCH  0
#define BLUSYS_VERSION_STRING "7.0.0"
```

---

## Commit strategy summary

One commit per phase. Commits in order:

1. `feat: create components/blusys/ shell (merged platform component)`
2. `refactor: relocate HAL under blusys/hal/ with panels/ extraction`
3. `refactor: fold services under blusys/hal/services/ (device→system rename)`
4. `refactor: relocate and restructure framework under blusys/framework/`
5. `build: merge CMakeLists into single blusys component`
6. `refactor: update all consumers to new blusys paths`
7. `ci: update lint rules for merged component layering`
8. `feat: rename main/integration/→main/platform/ in scaffold + docs + version bump to 7.0.0`

Each phase lands green (lint passes, representative examples build). If a
phase breaks something, fix-forward in the same phase's work before
committing.

---

## Sign-off

This document reflects the architecture agreed upon through iterative design
discussion. Any deviation during implementation must update this document
first.

**Architecture locked: 2026-04-14.**

# Blusys Platform Restructure — v0 Architecture

This document captures the complete v0 architecture for the Blusys platform and
the phased implementation plan to get there. It is the authoritative design
reference for this restructure.

---

## Context

The platform currently ships as three separate ESP-IDF components:

- `components/blusys_hal/` — C HAL with drivers mixed in (`drivers/` sub-dir)
- `components/blusys_services/` — C stateful runtime (Wi-Fi, storage, MQTT, …)
- `components/blusys_framework/` — C++ app framework

Problems with the current split:

- Products always require all three (transitive deps force it)
- Drivers for external components (buttons, LCDs, DHTs) live inside HAL,
  blurring the distinction between silicon wrappers and component interfaces
- Services are 95% ESP-IDF wrappers — as device-coupled as HAL, yet live
  in their own component
- The `framework/` internal layout splits a single concept (e.g. feedback)
  across `app/` and `core/` with no consistent axis
- Many singleton directories exist (`drivers/actuator/`, `widgets/button/button.hpp`,
  `ui/input/`, `ui/icons/`, `services/output/`, `services/input/`, …)
- Panel-specific data (ILI9341 is 240×320) lives in `framework/`, violating
  the "device-agnostic framework" principle
- Redundant `blusys_` prefixes everywhere (`blusys_global_lock.h` inside
  `blusys/internal/`, `blusys_wifi.c` inside `blusys/connectivity/`, …)

**Goal:** a single coherent platform, four clean layers with one-way
dependencies, concept-based internal organization, minimal naming noise,
architecturally honest, lint-enforced, and structured to stay maintainable
as the codebase grows.

---

## Design principles

These are the non-negotiable invariants of the v0 architecture.

### 1. One merged component, four sibling layers

All platform code lives in a single ESP-IDF component: `components/blusys/`.
Products depend on it with `REQUIRES blusys`. Internally, four sibling
layers with a strict one-way dependency flow:

```
hal/       → (nothing)
drivers/   → hal/
services/  → hal/ + drivers/
framework/ → (pure; only framework/platform/ may bridge to layers below)
```

- **`hal/`** — silicon peripheral wrappers (C). Thin ESP-IDF wrappers:
  GPIO, SPI, I2C, timers, ADC, etc.
- **`drivers/`** — external hardware component interfaces (C). Buttons,
  LCDs, encoders, DHT sensors, LED strips, panel data. Programs against
  HAL's C API.
- **`services/`** — stateful runtime modules (C). Wi-Fi, MQTT, OTA, FATFS,
  USB HID host, console. Built on ESP-IDF stacks.
- **`framework/`** — device-agnostic C++ app runtime. The only layer
  allowed to be product-facing beyond raw HAL.

This is architecturally honest: HAL is thin silicon, drivers compose HAL
for external components, services are ESP-IDF-bound runtime, framework is
portable C++.

### 2. `src/` mirrors `include/blusys/` exactly

For every public header at `include/blusys/X/Y/Z.h`, the corresponding source
lives at `src/X/Y/Z.c` (or `.cpp`). Enforceable by lint. Exceptions:

- Private headers colocated with their single consumer in `src/`
- Internal-only sources that have no public header (e.g., `src/hal/targets/`)

### 3. Concept-based internal organization

Within each layer, directories group by *concept*, not by implementation
axis. Every file has one answer to the question *"which concept does this
belong to?"*. No two-axis classification (API vs engine, declarative vs
imperative). Each concept owns its full stack: public types, specific
implementations, and internal helpers.

### 4. No singleton directories, but category slots are allowed

Flat wins over sub-dirs containing 1 file. Widgets, drivers, primitives —
all flat. Sub-directories are permitted at 2+ files when they represent a
real conceptual bucket that will grow (e.g., `ui/input/`, `ui/extension/`).
Speculative empty categories are forbidden.

### 5. `framework/platform/` is the sole escape hatch

Framework's `framework/platform/` is the ONLY zone allowed to `#include`
HAL / drivers / services. Pure framework (`app/`, `capabilities/`,
`flows/`, `engine/`, `feedback/`, `ui/`) must compile
without any lower-layer headers.

Product's `main/platform/` mirrors this role at product level.

### 6. Framework is truly device-agnostic

No panel dimensions, panel protocols, or silicon specifics hardcoded
under `framework/`. Panel hardware data (width, height, protocol defaults)
lives in `drivers/panels/`. Framework's platform profiles compose panel
data from drivers, not from hardcoded constants.

### 7. Minimal `blusys_` naming noise

- **Paths** carry the namespace: one `blusys/` at the top of every include
  path. File names inside never repeat the prefix.
- **C public symbols** (in `hal/`, `drivers/`, `services/`) keep the
  `blusys_` prefix because C has no namespaces.
- **C++ public symbols** (in `framework/`) live inside `namespace blusys { }`.
  Identifiers are naked — no `Blusys` prefix on classes, no `blusys_`
  prefix on functions.
- **Macros** keep `BLUSYS_*` (global scope).
- **Kconfig** keeps `CONFIG_BLUSYS_*` (flat namespace).

### 8. Layered umbrella headers, minimal set

Two umbrella headers, no per-layer ones:

- `blusys/blusys.h` — C consumers (hal + drivers + services)
- `blusys/blusys.hpp` — C++ consumers (everything)

Consumers who want finer granularity include specific headers.

### 9. Private headers stay local

A header included by exactly one `.cpp` lives next to that `.cpp` under
`src/`, not in a public `internal/`. Only cross-file internals live in
public `internal/` dirs — and those internal dirs live *inside their
owning concept* (`engine/internal/`, `feedback/internal/`, `hal/internal/`),
never as a top-level grab bag.

### 10. Product scaffold mirrors the framework

```
Product main/           Framework:
  core/          ↔       framework/app/, capabilities/, flows/, ...
  ui/            ↔       framework/ui/
  platform/      ↔       framework/platform/
```

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
│   ├── blusys.h                  ← C umbrella (hal + drivers + services)
│   ├── blusys.hpp                ← C++ umbrella (everything)
│   ├── hal/
│   ├── drivers/
│   ├── services/
│   └── framework/
└── src/
    ├── hal/        (mirrors include/blusys/hal/)
    ├── drivers/    (mirrors include/blusys/drivers/)
    ├── services/   (mirrors include/blusys/services/)
    └── framework/  (mirrors include/blusys/framework/)
```

### HAL layer

Silicon peripheral wrappers. Flat — no `peripheral/` sub-dir (HAL already
means "peripheral wrappers"; the nesting was pure noise).

```
include/blusys/hal/
├── adc.h  dac.h  efuse.h  error.h
├── gpio.h  gpio_expander.h
├── i2c.h  i2c_slave.h  i2s.h
├── log.h
├── mcpwm.h  nvs.h  one_wire.h
├── pcnt.h  pwm.h  rmt.h
├── sd_spi.h  sdm.h  sdmmc.h  sleep.h
├── spi.h  spi_slave.h
├── system.h  target.h  temp_sensor.h  timer.h
├── touch.h  twai.h  uart.h  ulp.h
├── usb_device.h  usb_host.h
├── version.h  wdt.h
└── internal/                     ← cross-file HAL internals (prefix dropped)
    ├── bt_stack.h                (was blusys_bt_stack.h)
    ├── esp_err_shim.h            (was blusys_esp_err.h — shim suffix avoids clash with ESP-IDF's esp_err.h)
    ├── global_lock.h             (was blusys_global_lock.h)
    ├── hal_internal.h            (was blusys_internal.h — avoids tautological internal/internal.h)
    ├── lock.h                    (was blusys_lock.h)
    ├── nvs_init.h                (was blusys_nvs_init.h)
    ├── panel_ili.h               (was lcd_panel_ili.h)
    ├── target_caps.h             (was blusys_target_caps.h)
    └── timeout.h                 (was blusys_timeout.h)
```

```
src/hal/
├── (peripheral .c files, mirroring include/)
├── internal/                     (cross-file internal sources)
│   ├── bt_stack.c                (was src/bt_stack.c)
│   └── lock_freertos.c           (was src/blusys_lock_freertos.c)
├── targets/                      ← per-MCU capability tables (SRC-ONLY)
│   ├── esp32/target_caps.c
│   ├── esp32s3/target_caps.c
│   └── esp32c3/target_caps.c
└── ulp/                          (ULP sources, SRC-ONLY)
```

`targets/` and `ulp/` have no public headers — internal capability tables
consumed only by other HAL sources.

### Drivers layer

External component interfaces. Flat except for `panels/` (a cohesive
family consumed by `lcd.h`).

```
include/blusys/drivers/
├── button.h  buzzer.h  dht.h  display.h
├── encoder.h  lcd.h  led_strip.h  seven_seg.h
└── panels/                       ← LCD panel HAL defaults family
    ├── ili9341.h  ili9488.h  st7735.h  st7789.h
    ├── ssd1306.h  qemu_rgb.h
```

**Notable placement decisions:**
- `display.h` moved from services — it's 90% render-path (framebuffer,
  pixel format, LVGL flush), not lifecycle.
- `panels/` is new — panel hardware data previously hardcoded in
  framework profiles.

### Services layer

Stateful runtime. Four sibling categories (no more nested
`connectivity/protocol/`, no more singleton `input/` or `output/`).

```
include/blusys/services/
├── connectivity/                 ← network/wireless transports + HID host
│   ├── wifi.h  wifi_prov.h  wifi_mesh.h
│   ├── bluetooth.h  ble_gatt.h  espnow.h
│   └── usb_hid.h                 ← multi-transport HID (USB + BLE)
│
├── protocol/                     ← application-layer protocols (sibling of connectivity)
│   ├── http_client.h  http_server.h  mqtt.h
│   ├── ws_client.h  mdns.h  sntp.h  local_ctrl.h
│
├── storage/
│   ├── fatfs.h  fs.h
│
└── system/                       ← renamed from device/
    ├── console.h  ota.h  power_mgmt.h
```

**Notable placement decisions:**
- `usb_hid` placed in `connectivity/` — it's a stateful multi-transport
  (USB + BLE) HID host, semantically a connectivity concern.
- `protocol/` promoted to sibling of `connectivity/` — MQTT runs over any
  transport; nesting it inside connectivity was conceptually wrong.
- `device/` → `system/` — files are `console`, `ota`, `power_mgmt`, all
  system lifecycle.
- Singleton `services/input/` and `services/output/` eliminated.

### Framework layer

Device-agnostic C++ app runtime. Concept-based internal organization.

```
include/blusys/framework/
├── framework.hpp                 ← framework-layer umbrella
├── callbacks.hpp                 ← generic callback type aliases (used framework-wide)
├── containers.hpp                ← static_vector, ring_buffer, string<N>
│
├── app/                          ← App type + immediate collaborators only
│   ├── app.hpp
│   ├── ctx.hpp                   (was app_ctx.hpp)
│   ├── spec.hpp                  (was app_spec.hpp)
│   ├── identity.hpp              (was app_identity.hpp)
│   ├── services.hpp              (was app_services.hpp)
│   ├── entry.hpp
│   ├── variant_dispatch.hpp      (reducer helper — stays with App)
│   └── internal/
│       └── app_runtime.hpp       (was app/detail/app_runtime.hpp)
│
├── capabilities/                 ← full capability system
│   ├── capabilities.hpp          ← umbrella
│   ├── capability.hpp            (was app/capability.hpp)
│   ├── event.hpp                 (was app/capability_event.hpp)
│   ├── list.hpp                  (was app/capability_list.hpp)
│   ├── inline.hpp                (was app/capabilities_inline.hpp)
│   ├── bluetooth.hpp  connectivity.hpp  diagnostics.hpp
│   ├── lan_control.hpp  mqtt_host.hpp  ota.hpp
│   ├── provisioning.hpp  storage.hpp
│   └── telemetry.hpp  usb.hpp
│
├── flows/                        ← lifecycle orchestration
│   ├── flows.hpp                 ← umbrella
│   ├── boot.hpp  loading.hpp  error.hpp
│   ├── status.hpp  settings.hpp
│   ├── connectivity_flow.hpp  provisioning_flow.hpp
│   ├── diagnostics_flow.hpp  ota_flow.hpp
│
├── engine/                       ← event queue, intent routing, dispatch (merged from routing + runtime)
│   ├── router.hpp                (was core/router.hpp)
│   ├── intent.hpp                (was core/intent.hpp)
│   ├── integration_dispatch.hpp  (was app/integration_dispatch.hpp)
│   ├── event_queue.hpp           (was core/runtime.hpp — renamed to avoid dir/file collision)
│   ├── pending_events.hpp        (was app/detail/pending_events.hpp)
│   └── internal/
│       └── default_route_sink.hpp (was app/detail/default_route_sink.hpp)
│
├── feedback/                     ← feedback system
│   ├── feedback.hpp              (was core/feedback.hpp)
│   ├── presets.hpp               (was core/feedback_presets.hpp)
│   └── internal/
│       └── logging_sink.hpp      (was app/detail/feedback_logging_sink.hpp)
│
├── ui/                           ← UI toolkit (PURE — no layer-below includes)
│   ├── style/                    ← design tokens: theme, fonts, icons, motion (merged from theme+icons+effects)
│   │   ├── theme.hpp             ← theme_tokens struct (colors, spacing, typography, motion, elevation)
│   │   ├── presets.hpp           (was app/theme_presets.hpp)
│   │   ├── fonts.hpp
│   │   ├── icon_set.hpp
│   │   ├── icon_set_minimal.hpp
│   │   ├── transition.hpp
│   │   └── interaction_effects.hpp  (was ui/visual_feedback.hpp — renamed to disambiguate from framework/feedback/)
│   ├── input/                    ← input abstractions (NOT widgets)
│   │   ├── encoder.hpp
│   │   └── focus_scope.hpp
│   ├── composition/              ← view structure + screen navigation
│   │   ├── view.hpp              (was app/view/view.hpp)
│   │   ├── page.hpp              (was app/view/page.hpp)
│   │   ├── shell.hpp             (was app/view/shell.hpp)
│   │   ├── overlay_manager.hpp   (was app/view/overlay_manager.hpp)
│   │   ├── screen_registry.hpp   (was app/view/screen_registry.hpp)
│   │   └── screen_router.hpp     (was app/view/screen_router.hpp; implements engine/route_sink)
│   ├── binding/                  ← widget ↔ state/event wiring
│   │   ├── bindings.hpp          (was app/view/bindings.hpp)
│   │   ├── action_widgets.hpp    (was app/view/action_widgets.hpp)
│   │   └── composites.hpp        (was app/view/composites.hpp)
│   ├── extension/                ← LVGL escape hatches
│   │   ├── custom_widget.hpp     (was app/view/custom_widget.hpp)
│   │   └── lvgl_scope.hpp        (was app/view/lvgl_scope.hpp)
│   ├── primitives/               ← layout + typography (unchanged internally)
│   │   ├── col.hpp  row.hpp  screen.hpp  label.hpp
│   │   ├── divider.hpp  icon_label.hpp  key_value.hpp  status_badge.hpp
│   ├── widgets/                  ← FLAT (was 17 singleton sub-dirs)
│   │   ├── button.hpp  card.hpp  chart.hpp  data_table.hpp
│   │   ├── dropdown.hpp  gauge.hpp  input_field.hpp  knob.hpp
│   │   ├── level_bar.hpp  list.hpp  modal.hpp  overlay.hpp
│   │   └── progress.hpp  slider.hpp  tabs.hpp  toggle.hpp  vu_strip.hpp
│   └── screens/                  ← stock screens (moved from app/screens/)
│       ├── about.hpp             (was about_screen.hpp — suffix dropped inside screens/)
│       ├── diagnostics.hpp
│       └── status.hpp
│
└── platform/                     ← framework ↔ HAL/drivers/services bridge (sole escape hatch)
    ├── profile.hpp               (was app/platform_profile.hpp)
    ├── host.hpp                  (was app/host_profile.hpp; absorbs profiles/host.hpp)
    ├── headless.hpp              (was app/profiles/headless.hpp)
    ├── auto.hpp                  (was app/auto_profile.hpp)
    ├── build.hpp                 (was app/build_profile.hpp)
    ├── reference_build.hpp       (was app/reference_build_profile.hpp)
    ├── layout_surface.hpp
    ├── auto_shell.hpp
    ├── button_array_bridge.hpp
    ├── input_bridge.hpp
    ├── touch_bridge.hpp
    └── profiles/                 ← optional convenience bundles (composes drivers/panels + layout)
        ├── ili9341_with_encoder.hpp
        ├── ssd1306_i2c.hpp
        └── ... (future)
```

### Umbrella headers (final list, two total)

- **`include/blusys/blusys.h`** — C umbrella. Includes all of `hal/`,
  `drivers/`, and `services/`. Use from C code.
- **`include/blusys/blusys.hpp`** — C++ umbrella. Includes everything.
  Use from C++ products.

No per-layer umbrellas (`hal.h`, `drivers.h`, `services.h`,
`framework.hpp`) beyond `framework/framework.hpp` which is kept as an
*internal* convenience for framework self-composition and is not a
product-facing contract. Products include specific headers or
`blusys.h` / `blusys.hpp`.

### Product scaffold

```
your_product/main/
├── core/         ← state + actions + reducer  (pure: framework only)
├── ui/           ← screens                    (pure: framework only)
├── platform/     ← hardware + app_main        (escape hatch: may include any layer)
├── CMakeLists.txt
└── idf_component.yml
```

The `main/integration/` → `main/platform/` rename aligns with the
framework's platform naming. Same word, same meaning, at every level.

---

## Naming policy

| Location | Naming | Rationale |
|---|---|---|
| Include paths | `blusys/...` (top-level only) | Namespace marker |
| File names | naked (no `blusys_` prefix) | Path provides context |
| C public symbols (hal, drivers, services) | `blusys_foo_init()`, `blusys_error_t` | C has no namespaces |
| C macros | `BLUSYS_OK`, `BLUSYS_LOG_I` | Global scope |
| C++ namespace | `namespace blusys { ... }` | C++ namespace marker |
| C++ identifiers (inside namespace) | naked (`App`, `Router`, `Ctx`) | Reached via `blusys::App` |
| Component target | `blusys` | CMake target |
| Kconfig | `CONFIG_BLUSYS_*` | Flat Kconfig namespace |

**Renames driven by policy (applied in Phase 5):**

- Every C++ class `Blusys*` → naked within `namespace blusys { }`.
- Every C++ typedef / function with `blusys_` prefix → naked within namespace.
- Every `hal/internal/blusys_*.h` → `hal/internal/*.h`.
- Every file reference inside `src/` updated to match.

---

## Lint-enforced invariants

`scripts/lint-layering.sh` enforces:

### Rule 1 — HAL cannot depend on layers above

```bash
grep -rEn --include='*.c' --include='*.h' \
    '#include[[:space:]]*[<"]blusys/(drivers|services|framework)/' \
    components/blusys/src/hal \
    components/blusys/include/blusys/hal \
    && exit 1
```

### Rule 2 — Drivers cannot depend on services or framework

```bash
grep -rEn --include='*.c' --include='*.h' \
    '#include[[:space:]]*[<"]blusys/(services|framework)/' \
    components/blusys/src/drivers \
    components/blusys/include/blusys/drivers \
    && exit 1
```

### Rule 3 — Services cannot depend on framework

```bash
grep -rEn --include='*.c' --include='*.h' \
    '#include[[:space:]]*[<"]blusys/framework/' \
    components/blusys/src/services \
    components/blusys/include/blusys/services \
    && exit 1
```

### Rule 4 — Pure framework cannot depend on lower layers

```bash
grep -rEn --include='*.hpp' --include='*.cpp' \
    --exclude-dir=platform \
    '#include[[:space:]]*[<"]blusys/(hal|drivers|services)/' \
    components/blusys/src/framework \
    components/blusys/include/blusys/framework \
    && exit 1
```

`framework/platform/` is the sole allowed exception.

### Rule 5 — `src/` must mirror `include/blusys/`

Python script (`scripts/check-src-include-mirror.py`) verifies for every
`src/X/Y/Z.{c,cpp}` there is a corresponding `include/blusys/X/Y/Z.{h,hpp}`,
and vice versa. Exceptions:
- Private headers colocated in `src/`
- `src/hal/targets/**`
- `src/hal/ulp/**`
- Concept umbrella headers (no `src/` counterpart expected):
  - `include/blusys/blusys.h`, `include/blusys/blusys.hpp`
  - `include/blusys/framework/framework.hpp`
  - `include/blusys/framework/capabilities/capabilities.hpp`
  - `include/blusys/framework/flows/flows.hpp`
  - `include/blusys/framework/ui/widgets.hpp`
  - `include/blusys/framework/ui/primitives.hpp`
  - `include/blusys/framework/ui/screens/screens.hpp`

### Rule 6 — No `blusys_` prefix inside file names under `blusys/`

Python script scans `include/blusys/**/*.{h,hpp}` and flags any file whose
basename starts with `blusys_`. (The path already carries the namespace.)

### Rule 7 — C++ identifiers live inside `namespace blusys`

Python script (`scripts/check-cpp-namespace.py`) verifies every `.hpp` /
`.cpp` under `src/framework` and `include/blusys/framework` contains
`namespace blusys {` exactly once at top level (or explicit
`namespace blusys::sub {`).

---

## Implementation plan

The restructure is broken into **8 phases**, each landing as a single
atomic commit. Phases must land in order.

### Phase 0 — Preparation

**Goal:** baseline snapshot and tooling.

- Capture file inventory: `find components/ -type f | sort > /tmp/inventory-before.txt`
- Identify all consumers of current paths (examples, docs, scripts, cmake)
- Dry-run the path migration script to validate all paths are accounted for

Output: confirmed migration list, no actions taken.

**Checklist:**

- [ ] File inventory captured to `/tmp/inventory-before.txt`
- [ ] Consumer audit complete (examples, docs, scripts, cmake)
- [ ] Migration script dry-run passes with zero unaccounted paths

### Phase 1 — Create the merged component shell

**Goal:** stand up `components/blusys/` with build skeleton.

Actions:
- Create `components/blusys/` directory
- Create minimal `CMakeLists.txt` with empty SRCS list — **populated incrementally across Phases 2–5** as each layer's sources land (not deferred to Phase 6)
- Create `Kconfig` merging the three existing Kconfigs
- Create `idf_component.yml` merging existing ones
- Create empty `include/blusys/` and `src/` directories

Deliverable: new component exists but compiles nothing. Each subsequent
phase (2, 3, 4, 5a–5d) appends its sources to `CMakeLists.txt` and must
leave the tree build-green before committing.

**Checklist:**

- [ ] `components/blusys/` directory created
- [ ] Minimal `CMakeLists.txt` with empty SRCS created
- [ ] Merged `Kconfig` created
- [ ] Merged `idf_component.yml` created
- [ ] Empty `include/blusys/` and `src/` directories created
- [ ] New blusys component registers successfully with ESP-IDF

### Phase 2 — Move HAL, flatten, and clean prefixes

**Goal:** relocate HAL under `blusys/hal/`, flatten `peripheral/`, drop
`blusys_` prefix from internal headers.

Git moves (preserve history):

```
components/blusys_hal/include/blusys/*.h           → components/blusys/include/blusys/hal/*.h
    (includes log.h)
components/blusys_hal/include/blusys/internal/*.h  → components/blusys/include/blusys/hal/internal/*.h  (renamed)
components/blusys_hal/src/*.c                      → components/blusys/src/hal/*.c
    (except blusys_lock_freertos.c and bt_stack.c — see below)
components/blusys_hal/src/blusys_lock_freertos.c   → components/blusys/src/hal/internal/lock_freertos.c
components/blusys_hal/src/bt_stack.c               → components/blusys/src/hal/internal/bt_stack.c
components/blusys_hal/src/targets/                 → components/blusys/src/hal/targets/
components/blusys_hal/src/ulp/                     → components/blusys/src/hal/ulp/
```

Additionally, **delete** the old C umbrella:
`components/blusys_hal/include/blusys/blusys.h` — superseded by the new
merged `include/blusys/blusys.h` created in Phase 1 (repopulated with
hal-only content now; extended per layer in later phases).

**Leave drivers in place for Phase 3** — they stay in
`components/blusys_hal/src/drivers/` and `include/blusys/drivers/` until
Phase 3 promotes them to a sibling layer.

Prefix drops and anti-clash renames in `hal/internal/`:

```
blusys_bt_stack.h     → bt_stack.h
blusys_esp_err.h      → esp_err_shim.h      (shim suffix — avoids clash with ESP-IDF's esp_err.h)
blusys_global_lock.h  → global_lock.h
blusys_internal.h     → hal_internal.h      (avoids tautological internal/internal.h)
blusys_lock.h         → lock.h
blusys_nvs_init.h     → nvs_init.h
blusys_target_caps.h  → target_caps.h
blusys_timeout.h      → timeout.h
lcd_panel_ili.h       → panel_ili.h
```

Intra-HAL include updates:
- `#include "blusys/gpio.h"` → `#include "blusys/hal/gpio.h"`
- `#include "blusys/internal/blusys_global_lock.h"` → `#include "blusys/hal/internal/global_lock.h"`
- `#include "blusys/internal/blusys_esp_err.h"` → `#include "blusys/hal/internal/esp_err_shim.h"`
- `#include "blusys/internal/blusys_internal.h"` → `#include "blusys/hal/internal/hal_internal.h"`
- etc.

Append hal sources to `components/blusys/CMakeLists.txt` and verify
`blusys_hal`-equivalent build succeeds before committing.

Remove: `components/blusys_hal/` *except* the `drivers/` sub-trees
(handled in Phase 3).

**Checklist:**

- [ ] Public HAL headers moved (incl. `log.h`) → `include/blusys/hal/`
- [ ] Internal headers moved with prefix drops + anti-clash renames
  (`esp_err_shim.h`, `hal_internal.h`) → `include/blusys/hal/internal/`
- [ ] Peripheral sources moved → `src/hal/`
- [ ] `bt_stack.c` moved → `src/hal/internal/bt_stack.c`
- [ ] `blusys_lock_freertos.c` moved → `src/hal/internal/lock_freertos.c`
- [ ] `src/targets/` moved → `src/hal/targets/`
- [ ] `src/ulp/` moved → `src/hal/ulp/`
- [ ] Old `blusys_hal/include/blusys/blusys.h` umbrella deleted
- [ ] Intra-HAL includes rewritten
- [ ] HAL sources appended to `CMakeLists.txt`
- [ ] Build-green (representative examples)
- [ ] `components/blusys_hal/` removed except `drivers/` sub-trees

### Phase 3 — Create drivers layer

**Goal:** promote drivers to a sibling layer, extract panel data, move
display driver from services.

Git moves:

```
components/blusys_hal/include/blusys/drivers/*.h  → components/blusys/include/blusys/drivers/*.h
components/blusys_hal/src/drivers/*.c             → components/blusys/src/drivers/*.c
components/blusys_services/include/blusys/output/display.h → components/blusys/include/blusys/drivers/display.h
components/blusys_services/src/output/display.c            → components/blusys/src/drivers/display.c
```

**NEW FILES** (panel hardware data extracted from framework):

```
components/blusys/include/blusys/drivers/panels/ili9341.h
components/blusys/include/blusys/drivers/panels/ili9488.h
components/blusys/include/blusys/drivers/panels/st7735.h
components/blusys/include/blusys/drivers/panels/st7789.h
components/blusys/include/blusys/drivers/panels/ssd1306.h
components/blusys/include/blusys/drivers/panels/qemu_rgb.h
```

Each contains panel HAL defaults only — dimensions, driver enum, protocol
config. No framework types.

Intra-drivers include path updates:
- `#include "blusys/drivers/lcd.h"` stays (path now includes drivers/ as its own layer)
- `#include "blusys/output/display.h"` → `#include "blusys/drivers/display.h"`

Append drivers sources to `components/blusys/CMakeLists.txt` and verify
hal + drivers build succeeds before committing.

Finally, remove the now-empty `components/blusys_hal/` directory.

**Checklist:**

- [ ] Drivers public headers moved → `include/blusys/drivers/`
- [ ] Drivers sources moved → `src/drivers/`
- [ ] `output/display.{h,c}` moved from services → `drivers/`
- [ ] New panel headers authored (`panels/{ili9341,ili9488,st7735,st7789,ssd1306,qemu_rgb}.h`)
- [ ] Intra-drivers includes rewritten
- [ ] Drivers sources appended to `CMakeLists.txt`
- [ ] Build-green
- [ ] `components/blusys_hal/` directory removed

### Phase 4 — Fold services with category cleanup

**Goal:** relocate services under `blusys/services/`, restructure
categories.

Git moves:

```
components/blusys_services/include/blusys/connectivity/       → components/blusys/include/blusys/services/connectivity/
components/blusys_services/include/blusys/connectivity/protocol/ → components/blusys/include/blusys/services/protocol/  (promoted to sibling)
components/blusys_services/include/blusys/storage/            → components/blusys/include/blusys/services/storage/
components/blusys_services/include/blusys/device/             → components/blusys/include/blusys/services/system/       (renamed)
components/blusys_services/include/blusys/input/usb_hid.h     → components/blusys/include/blusys/services/connectivity/usb_hid.h  (moved into connectivity)
components/blusys_services/src/connectivity/                  → components/blusys/src/services/connectivity/
components/blusys_services/src/connectivity/protocol/         → components/blusys/src/services/protocol/
components/blusys_services/src/storage/                       → components/blusys/src/services/storage/
components/blusys_services/src/device/                        → components/blusys/src/services/system/
components/blusys_services/src/input/usb_hid.c                → components/blusys/src/services/connectivity/usb_hid.c
```

(`components/blusys_services/{include,src}/output/display.*` already
moved to drivers in Phase 3.)

Intra-services include updates:
- `blusys/connectivity/wifi.h` → `blusys/services/connectivity/wifi.h`
- `blusys/connectivity/protocol/mqtt.h` → `blusys/services/protocol/mqtt.h`
- `blusys/device/ota.h` → `blusys/services/system/ota.h`
- `blusys/input/usb_hid.h` → `blusys/services/connectivity/usb_hid.h`

Delete old services umbrella `components/blusys_services/include/blusys/blusys_services.h`
— superseded by the merged `include/blusys/blusys.h` (extended here to
include services).

Append services sources to `components/blusys/CMakeLists.txt` and verify
hal + drivers + services build succeeds before committing.

Remove: `components/blusys_services/` entirely.

**Checklist:**

- [ ] `connectivity/` moved → `services/connectivity/`
- [ ] `connectivity/protocol/` promoted → `services/protocol/` (sibling)
- [ ] `storage/` moved → `services/storage/`
- [ ] `device/` renamed + moved → `services/system/`
- [ ] `input/usb_hid.{h,c}` moved → `services/connectivity/`
- [ ] Old `blusys_services.h` umbrella deleted
- [ ] Intra-services includes rewritten
- [ ] Services sources appended to `CMakeLists.txt`
- [ ] Build-green
- [ ] `components/blusys_services/` removed entirely

### Phase 5 — Move and restructure framework (four commits: 5a–5d)

**Goal:** relocate framework, restructure into concept-based layout,
wrap in `namespace blusys { }`, drop `blusys_` prefixes from C++
identifiers.

**Split into four independent commits.** Each commit fixes its own
`#include` paths and appends its touched sources to
`components/blusys/CMakeLists.txt`. Commits 5a, 5b, 5c must land
build-green. Commit 5d is the one step where intermediate states may not
build; land it as a single atomic sweep.

---

**Commit 5a — Concept restructure (path relocations, non-UI):**

```
components/blusys_framework/include/blusys/app/app.hpp              → framework/app/app.hpp
components/blusys_framework/include/blusys/app/app_ctx.hpp          → framework/app/ctx.hpp
components/blusys_framework/include/blusys/app/app_spec.hpp         → framework/app/spec.hpp
components/blusys_framework/include/blusys/app/app_identity.hpp     → framework/app/identity.hpp
components/blusys_framework/include/blusys/app/app_services.hpp     → framework/app/services.hpp
components/blusys_framework/include/blusys/app/entry.hpp            → framework/app/entry.hpp
components/blusys_framework/include/blusys/app/variant_dispatch.hpp → framework/app/variant_dispatch.hpp
components/blusys_framework/include/blusys/app/detail/app_runtime.hpp → framework/app/internal/app_runtime.hpp

components/blusys_framework/include/blusys/app/capability*.hpp      → framework/capabilities/{capability,event,list,inline}.hpp
components/blusys_framework/include/blusys/app/capabilities/*.hpp   → framework/capabilities/*.hpp
components/blusys_framework/include/blusys/app/flows/*.hpp          → framework/flows/*.hpp

components/blusys_framework/include/blusys/framework/core/router.hpp   → framework/engine/router.hpp
components/blusys_framework/include/blusys/framework/core/intent.hpp   → framework/engine/intent.hpp
components/blusys_framework/include/blusys/app/integration_dispatch.hpp → framework/engine/integration_dispatch.hpp
components/blusys_framework/include/blusys/framework/core/runtime.hpp  → framework/engine/event_queue.hpp
components/blusys_framework/include/blusys/app/detail/pending_events.hpp → framework/engine/pending_events.hpp
components/blusys_framework/include/blusys/app/detail/default_route_sink.hpp → framework/engine/internal/default_route_sink.hpp

components/blusys_framework/include/blusys/framework/core/feedback.hpp         → framework/feedback/feedback.hpp
components/blusys_framework/include/blusys/framework/core/feedback_presets.hpp → framework/feedback/presets.hpp
components/blusys_framework/include/blusys/app/detail/feedback_logging_sink.hpp → framework/feedback/internal/logging_sink.hpp

components/blusys_framework/include/blusys/framework/core/containers.hpp → framework/containers.hpp
components/blusys_framework/include/blusys/framework/ui/callbacks.hpp    → framework/callbacks.hpp

components/blusys_framework/include/blusys/app/theme_presets.hpp      → framework/ui/style/presets.hpp
```

Additionally in 5a:
- **Delete** `components/blusys_framework/include/blusys/app/integration.hpp`
  (its contents are an umbrella over files moving to `framework/platform/` in
  5b; the new `framework/framework.hpp` replaces it.)
- **Delete** the old `components/blusys_framework/include/blusys/framework/framework.hpp`
  and write a fresh `include/blusys/framework/framework.hpp` that includes
  the new concept dirs (`app/`, `capabilities/`, `flows/`, `engine/`,
  `feedback/`, `ui/`, `platform/`).
- Rewrite every `#include "blusys/app/..."` / `"blusys/framework/core/..."`
  touched by this commit to the new paths (scripted find+replace per
  migration tables).
- Append touched sources to `components/blusys/CMakeLists.txt`; verify build.

**Checklist (5a):**

- [ ] `app/*.hpp` moved → `framework/app/*` (with renames: ctx, spec, identity, services)
- [ ] `app/detail/app_runtime.hpp` → `framework/app/internal/`
- [ ] `app/capability*.hpp` + `app/capabilities/*` → `framework/capabilities/`
- [ ] `app/flows/*` → `framework/flows/*`
- [ ] `framework/core/{router,intent}.hpp` + `app/integration_dispatch.hpp` → `framework/engine/`
- [ ] `framework/core/runtime.hpp` → `framework/engine/event_queue.hpp`
- [ ] `app/detail/{pending_events,default_route_sink}.hpp` → `framework/engine/` (+ `internal/`)
- [ ] `framework/core/{feedback,feedback_presets}.hpp` → `framework/feedback/`
- [ ] `app/detail/feedback_logging_sink.hpp` → `framework/feedback/internal/`
- [ ] `framework/core/containers.hpp` → `framework/containers.hpp`
- [ ] `framework/ui/callbacks.hpp` → `framework/callbacks.hpp`
- [ ] `app/theme_presets.hpp` → `framework/ui/style/presets.hpp`
- [ ] `app/integration.hpp` deleted
- [ ] Old `framework/framework.hpp` deleted + fresh umbrella written
- [ ] Touched includes rewritten
- [ ] Sources appended to `CMakeLists.txt`
- [ ] Build-green

**Commit 5b — `integration/` → `platform/`:**

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
framework/app/profiles/{ili9341,ili9488,st7735,st7789,ssd1306,qemu_rgb}.hpp → framework/platform/profiles/*.hpp
```

Panel profile `.hpp` files are REWRITTEN to use `blusys/drivers/panels/*.h`
defaults rather than hardcoding dimensions.

Rewrite includes (`blusys/app/platform_profile.hpp` →
`blusys/framework/platform/profile.hpp`, etc.), append touched sources to
CMakeLists, verify build before committing.

**Checklist (5b):**

- [ ] `platform_profile.hpp` → `platform/profile.hpp`
- [ ] `host_profile.hpp` + `profiles/host.hpp` merged → `platform/host.hpp`
- [ ] `auto_profile.hpp` → `platform/auto.hpp`
- [ ] `build_profile.hpp` → `platform/build.hpp`
- [ ] `reference_build_profile.hpp` → `platform/reference_build.hpp`
- [ ] `layout_surface.hpp`, `auto_shell.hpp`, `{button_array,input,touch}_bridge.hpp` → `platform/`
- [ ] `profiles/headless.hpp` → `platform/headless.hpp`
- [ ] Panel profiles (`ili9341.hpp` etc.) → `platform/profiles/`
- [ ] Panel profile `.hpp` files rewritten against `drivers/panels/*.h` defaults
- [ ] Touched includes rewritten
- [ ] Sources appended to `CMakeLists.txt`
- [ ] Build-green

**Commit 5c — UI sub-dir reorganization:**

Move view/composition files (was `app/view/`) into `ui/composition/`:

```
framework/app/view/view.hpp             → framework/ui/composition/view.hpp
framework/app/view/page.hpp             → framework/ui/composition/page.hpp
framework/app/view/shell.hpp            → framework/ui/composition/shell.hpp
framework/app/view/overlay_manager.hpp  → framework/ui/composition/overlay_manager.hpp
framework/app/view/screen_registry.hpp  → framework/ui/composition/screen_registry.hpp
framework/app/view/screen_router.hpp    → framework/ui/composition/screen_router.hpp
```

Move binding files into `ui/binding/`:

```
framework/app/view/bindings.hpp        → framework/ui/binding/bindings.hpp
framework/app/view/action_widgets.hpp  → framework/ui/binding/action_widgets.hpp
framework/app/view/composites.hpp      → framework/ui/binding/composites.hpp
```

Move extension files into `ui/extension/`:

```
framework/app/view/custom_widget.hpp   → framework/ui/extension/custom_widget.hpp
framework/app/view/lvgl_scope.hpp      → framework/ui/extension/lvgl_scope.hpp
```

Regroup into `style/` (merges theme + icons + effects, with rename to disambiguate feedback):

```
framework/ui/theme.hpp                  → framework/ui/style/theme.hpp
framework/ui/fonts.hpp                  → framework/ui/style/fonts.hpp
framework/ui/icons/icon_set.hpp         → framework/ui/style/icon_set.hpp
framework/ui/icons/icon_set_minimal.hpp → framework/ui/style/icon_set_minimal.hpp
framework/ui/transition.hpp             → framework/ui/style/transition.hpp
framework/ui/visual_feedback.hpp        → framework/ui/style/interaction_effects.hpp   (RENAMED)
framework/ui/input/*                    → framework/ui/input/*   (path stable; dir name kept)
```

The `visual_feedback.hpp` → `interaction_effects.hpp` rename disambiguates
from `framework/feedback/` (the app feedback bus). `interaction_effects.hpp`
now clearly names press/focus/hover visual effects.

Flatten widget singleton dirs:

```
framework/ui/widgets/button/button.hpp  → framework/ui/widgets/button.hpp
... (17 widgets total)
```

Move stock screens to `ui/screens/` with `_screen` suffix dropped:

```
framework/app/screens/about_screen.hpp       → framework/ui/screens/about.hpp
framework/app/screens/diagnostics_screen.hpp → framework/ui/screens/diagnostics.hpp
framework/app/screens/status_screen.hpp      → framework/ui/screens/status.hpp
framework/app/screens/screens.hpp            → framework/ui/screens/screens.hpp (umbrella)
```

Move private UI headers out of the public surface (per principle 9 —
private headers stay local to their consumer):

```
framework/ui/detail/fixed_slot_pool.hpp → src/framework/ui/fixed_slot_pool.hpp
framework/ui/detail/flex_layout.hpp     → src/framework/ui/flex_layout.hpp
framework/ui/detail/widget_common.hpp   → src/framework/ui/widget_common.hpp
```

Delete the now-empty `framework/ui/detail/` directory.

**Umbrella retention:** `framework/ui/widgets.hpp` and
`framework/ui/primitives.hpp` are **kept** as concept-level umbrellas
(same category as `capabilities/capabilities.hpp`, `flows/flows.hpp`).
They are whitelisted in lint Rule 5.

Rewrite includes for everything moved in this commit, append touched
sources to CMakeLists, verify build before committing.

**Checklist (5c):**

- [ ] `app/view/{view,page,shell,overlay_manager,screen_registry,screen_router}.hpp` → `ui/composition/`
- [ ] `app/view/{bindings,action_widgets,composites}.hpp` → `ui/binding/`
- [ ] `app/view/{custom_widget,lvgl_scope}.hpp` → `ui/extension/`
- [ ] `ui/{theme,fonts,transition}.hpp` → `ui/style/`
- [ ] `ui/icons/*` → `ui/style/`
- [ ] `ui/visual_feedback.hpp` → `ui/style/interaction_effects.hpp` (renamed)
- [ ] 17 widget singleton dirs flattened (`widgets/button/button.hpp` → `widgets/button.hpp` × 17)
- [ ] `app/screens/*_screen.hpp` → `ui/screens/*.hpp` (suffix dropped)
- [ ] `app/screens/screens.hpp` → `ui/screens/screens.hpp`
- [ ] `ui/detail/{fixed_slot_pool,flex_layout,widget_common}.hpp` → `src/framework/ui/`
- [ ] Empty `ui/detail/` directory deleted
- [ ] `ui/widgets.hpp` and `ui/primitives.hpp` retained as concept umbrellas
- [ ] Touched includes rewritten
- [ ] Sources appended to `CMakeLists.txt`
- [ ] Build-green

**Commit 5d — C++ namespace and naming refactor:**

Single atomic sweep — intermediate states may not compile.

- Wrap every `.hpp` / `.cpp` under `framework/` in `namespace blusys { }`
- Rename identifiers dropping prefixes:
  - `class BlusysApp` → `class App` (inside `namespace blusys`)
  - `blusys_runtime_t` → `Runtime`
  - `blusys_intent_*` → `intent::*` or `Intent`
  - etc.
- Update all internal framework references to the new naked names
- Update all `framework/src/**` files to match
- Run the lint Rule 7 check (`scripts/check-cpp-namespace.py`) before committing.
- Verify representative examples build.

After 5d lands, remove `components/blusys_framework/` entirely.

**Checklist (5d):**

- [ ] Every `.hpp`/`.cpp` under `framework/` wrapped in `namespace blusys { }`
- [ ] `class Blusys*` → naked class names
- [ ] `blusys_*_t` typedefs → naked (`Runtime`, `Intent`, etc.)
- [ ] `blusys_*()` free functions → naked within namespace
- [ ] All framework-internal references updated to naked names
- [ ] `framework/src/**` updated to match
- [ ] `scripts/check-cpp-namespace.py` passes
- [ ] Representative examples build
- [ ] `components/blusys_framework/` removed entirely

### Phase 6 — Build system consolidation

**Goal:** consolidate the incrementally-populated `CMakeLists.txt` and
update auxiliary cmake helpers. (Main SRCS list has been appended to
across Phases 2–5d; this phase cleans up and finalizes.)

Actions:
- Reorganize `components/blusys/CMakeLists.txt` by layer with clean
  sectioning; verify it still builds
- Consolidate dependencies from the three old CMakeLists (any leftover
  REQUIRES merging)
- Finalize conditional per-target code (`src/hal/targets/`, `src/hal/ulp/`)
- Finalize conditional managed components (tinyusb, esp_lcd_qemu_rgb)
- Finalize LVGL conditional compilation for `framework/ui/`
- Update `cmake/blusys_host_bridge.cmake` paths to new layout
- Retire `cmake/blusys_framework_ui_sources.cmake` (the flat widget
  layout makes fine-grained listing unnecessary)
- Audit `cmake/blusys_framework_host_mqtt.cmake`,
  `cmake/blusys_framework_host_widgetkit.cmake`, and
  `cmake/blusys_optional_component.cmake` for stale paths

**Checklist:**

- [ ] `CMakeLists.txt` reorganized by layer with clean sectioning
- [ ] Dependencies consolidated from three old CMakeLists
- [ ] Per-target code (`hal/targets/`, `hal/ulp/`) finalized
- [ ] Managed components (tinyusb, esp_lcd_qemu_rgb) finalized
- [ ] LVGL conditional compilation finalized for `framework/ui/`
- [ ] `cmake/blusys_host_bridge.cmake` paths updated
- [ ] `cmake/blusys_framework_ui_sources.cmake` retired
- [ ] Host MQTT, widgetkit, optional_component cmake helpers audited
- [ ] Full build passes on all representative targets

### Phase 7 — Consumer update sweep

**Goal:** update every reference outside `components/blusys/`.

Files to audit and update:
- All `examples/**/*.{c,cpp,hpp,cmake}` include paths
- All `examples/**/main/CMakeLists.txt` (REQUIRES changes from
  `blusys_hal blusys_services blusys_framework` → `blusys`)
- All `examples/**/main/idf_component.yml`
- All `docs/**/*.md` references
- All `scripts/*.sh` and `scripts/*.py` that reference component paths
- `inventory.yml` if it references old paths
- `README.md` architecture diagram
- `CLAUDE.md` or other top-level instructions

Master search list:

```bash
rg 'blusys_hal'
rg 'blusys_services'
rg 'blusys_framework'
rg 'blusys/app/'
rg 'blusys/framework/core'
rg 'blusys/framework/ui/input/'
rg 'blusys/framework/ui/icons/'
rg 'blusys/framework/ui/widgets/[a-z_]+/[a-z_]+\.hpp'
rg 'blusys/drivers/'                # old HAL drivers path (now at blusys/drivers/, not under hal/)
rg 'blusys/connectivity/'           # old services path
rg 'blusys/connectivity/protocol/'  # old nested path
rg 'blusys/storage/'                # old services path
rg 'blusys/device/'                 # old services path (now system)
rg 'blusys/input/'                  # old services path
rg 'blusys/output/'                 # old services path (display → drivers, usb_hid → connectivity)
rg 'blusys/internal/'               # old HAL internal path
rg 'blusys_[a-z_]+\.h'              # old prefixed filenames
```

Every hit gets updated in Phase 7.

**Checklist:**

- [ ] `examples/**/*.{c,cpp,hpp,cmake}` include paths updated
- [ ] `examples/**/main/CMakeLists.txt` REQUIRES updated to `blusys`
- [ ] `examples/**/main/idf_component.yml` updated
- [ ] `docs/**/*.md` references updated
- [ ] `scripts/*.{sh,py}` component-path references updated
- [ ] `inventory.yml` updated
- [ ] `README.md` architecture diagram updated
- [ ] `CLAUDE.md` / top-level instructions updated (if present)
- [ ] Every `rg` in master search list returns zero hits

### Phase 8 — Lint, CI, scaffold, docs, version bump

**Goal:** machine-enforce invariants, ship the breaking change.

Actions:
- Rewrite `scripts/lint-layering.sh` with Rules 1–4 above
- Add `scripts/check-src-include-mirror.py` (Rule 5)
- Add `scripts/check-no-blusys-prefix.py` (Rule 6)
- Add `scripts/check-cpp-namespace.py` (Rule 7)
- Add CI job `blusys-layer-invariants` that runs all layering checks
- Update `scripts/check-host-bridge-spine.py` with new paths
- Rename `main/integration/` → `main/platform/` in scaffold templates
- Update scaffold-emitted include paths to new layout
- Update `blusys create --list` descriptions if needed
- Update `docs/start/`, `docs/app/`, `docs/hal/`, `docs/services/`
- Update `docs/internals/architecture.md` with new 4-layer diagram
- Bump version in `include/blusys/hal/version.h` to 7.0.0 (breaking
  change — major version bump)

**Checklist:**

- [ ] `scripts/lint-layering.sh` rewritten with Rules 1–4
- [ ] `scripts/check-src-include-mirror.py` added (Rule 5 with allowlist)
- [ ] `scripts/check-no-blusys-prefix.py` added (Rule 6)
- [ ] `scripts/check-cpp-namespace.py` added (Rule 7)
- [ ] CI job `blusys-layer-invariants` added
- [ ] `scripts/check-host-bridge-spine.py` updated
- [ ] Scaffold templates renamed `main/integration/` → `main/platform/`
- [ ] Scaffold-emitted include paths updated
- [ ] `blusys create --list` descriptions updated
- [ ] `docs/{start,app,hal,services}/` updated
- [ ] `docs/internals/architecture.md` updated with 4-layer diagram
- [ ] Version bumped to 7.0.0 in `include/blusys/hal/version.h`

---

## Migration reference tables

### HAL (peripherals flat, prefix dropped)

| Old | New |
|---|---|
| `blusys/gpio.h` | `blusys/hal/gpio.h` |
| `blusys/spi.h` | `blusys/hal/spi.h` |
| `blusys/log.h` | `blusys/hal/log.h` |
| `blusys/internal/blusys_global_lock.h` | `blusys/hal/internal/global_lock.h` |
| `blusys/internal/blusys_esp_err.h` | `blusys/hal/internal/esp_err_shim.h` (avoids clash with ESP-IDF `esp_err.h`) |
| `blusys/internal/blusys_internal.h` | `blusys/hal/internal/hal_internal.h` (avoids tautological `internal/internal.h`) |
| `blusys/internal/blusys_target_caps.h` | `blusys/hal/internal/target_caps.h` |
| `blusys/internal/lcd_panel_ili.h` | `blusys/hal/internal/panel_ili.h` |

### Drivers (promoted to sibling, display imported)

| Old | New |
|---|---|
| `blusys/drivers/lcd.h` | `blusys/drivers/lcd.h` (same path, different component — now under `blusys/drivers/` directly) |
| `blusys/output/display.h` | `blusys/drivers/display.h` |
| *(new)* | `blusys/drivers/panels/ili9341.h` |
| *(new)* | `blusys/drivers/panels/ili9488.h` |
| *(new)* | `blusys/drivers/panels/st7735.h` |
| *(new)* | `blusys/drivers/panels/st7789.h` |
| *(new)* | `blusys/drivers/panels/ssd1306.h` |
| *(new)* | `blusys/drivers/panels/qemu_rgb.h` |

### Services

| Old | New |
|---|---|
| `blusys/connectivity/wifi.h` | `blusys/services/connectivity/wifi.h` |
| `blusys/connectivity/protocol/mqtt.h` | `blusys/services/protocol/mqtt.h` |
| `blusys/storage/fatfs.h` | `blusys/services/storage/fatfs.h` |
| `blusys/device/console.h` | `blusys/services/system/console.h` |
| `blusys/device/ota.h` | `blusys/services/system/ota.h` |
| `blusys/device/power_mgmt.h` | `blusys/services/system/power_mgmt.h` |
| `blusys/input/usb_hid.h` | `blusys/services/connectivity/usb_hid.h` |
| `blusys/output/display.h` | (moved to drivers — see above) |

### Framework app

| Old | New |
|---|---|
| `blusys/app/app.hpp` | `blusys/framework/app/app.hpp` |
| `blusys/app/app_ctx.hpp` | `blusys/framework/app/ctx.hpp` |
| `blusys/app/app_spec.hpp` | `blusys/framework/app/spec.hpp` |
| `blusys/app/app_services.hpp` | `blusys/framework/app/services.hpp` |
| `blusys/app/app_identity.hpp` | `blusys/framework/app/identity.hpp` |
| `blusys/app/entry.hpp` | `blusys/framework/app/entry.hpp` |
| `blusys/app/variant_dispatch.hpp` | `blusys/framework/app/variant_dispatch.hpp` |
| `blusys/app/detail/app_runtime.hpp` | `blusys/framework/app/internal/app_runtime.hpp` |

### Framework capabilities

| Old | New |
|---|---|
| `blusys/app/capability.hpp` | `blusys/framework/capabilities/capability.hpp` |
| `blusys/app/capability_event.hpp` | `blusys/framework/capabilities/event.hpp` |
| `blusys/app/capability_list.hpp` | `blusys/framework/capabilities/list.hpp` |
| `blusys/app/capabilities_inline.hpp` | `blusys/framework/capabilities/inline.hpp` |
| `blusys/app/capabilities/connectivity.hpp` | `blusys/framework/capabilities/connectivity.hpp` |
| `blusys/app/capabilities/mqtt_host.hpp` | `blusys/framework/capabilities/mqtt_host.hpp` |
| (etc. for all 11 capabilities) | |

### Framework flows

| Old | New |
|---|---|
| `blusys/app/flows/boot.hpp` | `blusys/framework/flows/boot.hpp` |
| (etc. for all flows) | |

### Framework engine / feedback

| Old | New |
|---|---|
| `blusys/framework/core/router.hpp` | `blusys/framework/engine/router.hpp` |
| `blusys/framework/core/intent.hpp` | `blusys/framework/engine/intent.hpp` |
| `blusys/app/integration_dispatch.hpp` | `blusys/framework/engine/integration_dispatch.hpp` |
| `blusys/framework/core/runtime.hpp` | `blusys/framework/engine/event_queue.hpp` |
| `blusys/app/detail/pending_events.hpp` | `blusys/framework/engine/pending_events.hpp` |
| `blusys/app/detail/default_route_sink.hpp` | `blusys/framework/engine/internal/default_route_sink.hpp` |
| `blusys/framework/core/feedback.hpp` | `blusys/framework/feedback/feedback.hpp` |
| `blusys/framework/core/feedback_presets.hpp` | `blusys/framework/feedback/presets.hpp` |
| `blusys/app/detail/feedback_logging_sink.hpp` | `blusys/framework/feedback/internal/logging_sink.hpp` |
| `blusys/framework/core/containers.hpp` | `blusys/framework/containers.hpp` |
| `blusys/framework/ui/callbacks.hpp` | `blusys/framework/callbacks.hpp` |

### Framework UI

| Old | New |
|---|---|
| `blusys/framework/ui/theme.hpp` | `blusys/framework/ui/style/theme.hpp` |
| `blusys/framework/ui/fonts.hpp` | `blusys/framework/ui/style/fonts.hpp` |
| `blusys/app/theme_presets.hpp` | `blusys/framework/ui/style/presets.hpp` |
| `blusys/framework/ui/icons/icon_set.hpp` | `blusys/framework/ui/style/icon_set.hpp` |
| `blusys/framework/ui/icons/icon_set_minimal.hpp` | `blusys/framework/ui/style/icon_set_minimal.hpp` |
| `blusys/framework/ui/transition.hpp` | `blusys/framework/ui/style/transition.hpp` |
| `blusys/framework/ui/visual_feedback.hpp` | `blusys/framework/ui/style/interaction_effects.hpp` (renamed) |
| `blusys/framework/ui/input/encoder.hpp` | `blusys/framework/ui/input/encoder.hpp` (unchanged) |
| `blusys/framework/ui/widgets/button/button.hpp` | `blusys/framework/ui/widgets/button.hpp` |
| (etc. for all 17 widgets) | |
| `blusys/app/view/view.hpp` | `blusys/framework/ui/composition/view.hpp` |
| `blusys/app/view/page.hpp` | `blusys/framework/ui/composition/page.hpp` |
| `blusys/app/view/shell.hpp` | `blusys/framework/ui/composition/shell.hpp` |
| `blusys/app/view/overlay_manager.hpp` | `blusys/framework/ui/composition/overlay_manager.hpp` |
| `blusys/app/view/screen_registry.hpp` | `blusys/framework/ui/composition/screen_registry.hpp` |
| `blusys/app/view/screen_router.hpp` | `blusys/framework/ui/composition/screen_router.hpp` |
| `blusys/app/view/bindings.hpp` | `blusys/framework/ui/binding/bindings.hpp` |
| `blusys/app/view/action_widgets.hpp` | `blusys/framework/ui/binding/action_widgets.hpp` |
| `blusys/app/view/composites.hpp` | `blusys/framework/ui/binding/composites.hpp` |
| `blusys/app/view/custom_widget.hpp` | `blusys/framework/ui/extension/custom_widget.hpp` |
| `blusys/app/view/lvgl_scope.hpp` | `blusys/framework/ui/extension/lvgl_scope.hpp` |
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
| `blusys/app/profiles/headless.hpp` | `blusys/framework/platform/headless.hpp` |
| `blusys/app/profiles/ili9341.hpp` | `blusys/framework/platform/profiles/ili9341.hpp` |
| (etc. for all panel profiles) | |

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

# 2. src/include mirror check
python3 scripts/check-src-include-mirror.py

# 3. Filename prefix check
python3 scripts/check-no-blusys-prefix.py

# 4. C++ namespace check
python3 scripts/check-cpp-namespace.py

# 5. Stale-path audit
rg 'blusys_hal'             --glob '*.{c,h,hpp,cpp,cmake,txt,md,yml}'
rg 'blusys_services'        --glob '*.{c,h,hpp,cpp,cmake,txt,md,yml}'
rg 'blusys_framework'       --glob '*.{c,h,hpp,cpp,cmake,txt,md,yml}'
rg 'blusys/app/'            --glob '*.{c,h,hpp,cpp}'
rg 'blusys/framework/core'  --glob '*.{c,h,hpp,cpp}'
rg 'blusys/connectivity/'   --glob '*.{c,h,hpp,cpp}'
rg 'blusys/output/'         --glob '*.{c,h,hpp,cpp}'
rg 'blusys/input/'          --glob '*.{c,h,hpp,cpp}'
rg 'blusys/device/'         --glob '*.{c,h,hpp,cpp}'
rg 'widgets/[a-z_]+/[a-z_]+\.hpp' --glob '*.{c,h,hpp,cpp}'

# 6. Full builds (representative examples)
blusys build examples/quickstart/hello_blusys esp32s3
blusys build examples/quickstart/hello_blusys esp32c3
blusys build examples/quickstart/hello_blusys esp32
blusys build examples/reference/interactive_dashboard esp32s3
blusys host-build examples/quickstart/hello_blusys

# 7. Host-bridge spine
python3 scripts/check-host-bridge-spine.py

# 8. Scaffold sanity
blusys create --dry-run my_test_product
```

All must pass before committing each phase.

---

## Non-goals / explicit deferrals

- **Pulling "device-agnostic business logic" out of services.** The few
  files with portable code (`ble_gatt.c`, `local_ctrl.c`, `usb_hid.c`)
  are 95% ESP-IDF-glued. Extracting them is not worth the complexity
  for v0.
- **Test directory structure.** When tests are added, they go in
  `components/blusys/tests/` mirroring `src/`. No convention is codified
  until the first test lands.
- **Splitting `services/` into further categories.** The current four
  (`connectivity/`, `protocol/`, `storage/`, `system/`) cover all files.
  Future categories land when a real file doesn't fit.
- **Extension mechanism for external capabilities.** Products can already
  create capabilities in `main/` or in private components. No framework
  change needed.
- **C++ concepts / traits / virtual interfaces for HAL abstraction.**
  The HAL's C API surface IS the interface. A C++ trait layer gains
  nothing and costs clarity.

---

## Risks and mitigations

| Risk | Mitigation |
|---|---|
| Phase 5 is very large | Intentional — intermediate states don't build. Document thoroughly in commit message. |
| Examples and docs drift while restructure is in progress | Phase 7 is a single sweep; nothing lands until all consumers are green. |
| Breaking change for downstream teams | v6 → v7.0 means breaking changes are acceptable; document migration in CHANGELOG and README. |
| Lost git history on renames | Use `git mv` for every rename. `git log --follow` still works. |
| Panel profile rewrite breaks product assumptions | Panel `.hpp` files in `framework/platform/profiles/` produce the same `device_profile` output; only internals change. Product API unchanged. |
| C++ namespace refactor introduces subtle link-time issues | Lint rule 7 verifies every framework TU is wrapped. Build-test on all representative examples before commit. |
| Old include paths referenced in private forks | Out of scope. v7.0 breaking change is the contract. |

---

## Versioning

Current version: **6.1.1**.

After this restructure: **7.0.0** (major version bump for breaking API
paths, naming policy, and layer model).

Update `components/blusys/include/blusys/hal/version.h`:

```c
#define BLUSYS_VERSION_MAJOR  7
#define BLUSYS_VERSION_MINOR  0
#define BLUSYS_VERSION_PATCH  0
#define BLUSYS_VERSION_STRING "7.0.0"
```

---

## Commit strategy summary

One commit per phase. Phase 5 is split into four commits (5a–5d) for
reviewability and bisection. Commits in order:

1. `feat: create components/blusys/ shell (merged platform component)`
2. `refactor: relocate HAL flat under blusys/hal/ with prefix/clash-avoidance renames`
3. `refactor: promote drivers to blusys/drivers/, extract panels family, move display from services`
4. `refactor: fold services under blusys/services/ with category cleanup (protocol sibling, usb_hid→connectivity, device→system)`
5. 5a. `refactor(framework): concept restructure — app/capabilities/flows/engine/feedback paths`
6. 5b. `refactor(framework): integration/ → platform/ with panel profile rewrite against drivers/panels/`
7. 5c. `refactor(framework): UI regroup — style/, flat widgets, screens move, visual_feedback→interaction_effects, detail→src`
8. 5d. `refactor(framework): wrap in namespace blusys{} and drop C++ blusys_/Blusys prefixes`
9. `build: consolidate merged blusys CMakeLists and update cmake helpers`
10. `refactor: update all consumers to new blusys paths`
11. `ci: enforce layering/mirror/prefix/namespace invariants; rename main/integration→main/platform; bump to 7.0.0`

Each commit lands green (lint passes, representative examples build) —
**except 5d**, which is a single atomic namespace/naming sweep where
intermediate states may not compile. If any other commit breaks
something, fix-forward within that commit's work before landing.

---

## Sign-off

This document reflects the architecture agreed upon through iterative design
discussion. Any deviation during implementation must update this document
first.

**Architecture locked: 2026-04-14** (amended same day with file-mapping
gaps, anti-clash renames, incremental CMake, Phase 5 four-way split, and
lint Rule 5 umbrella allowlist).

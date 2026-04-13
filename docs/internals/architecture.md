# Architecture

This page describes the current repository architecture and tiering constraints.

For the product model and locked decisions, see `../README.md` (**Product foundations**).

Blusys is an internal ESP32 product platform. The repo ships three ESP-IDF components
sharing the `blusys/` header namespace:

```text
components/blusys_hal/            HAL + drivers
components/blusys_services/   runtime services
components/blusys_framework/  framework infrastructure (C++, widget kit, core spine)
```

The public include namespace stays `blusys/...` across all tiers.

## Guiding Principles

- keep the public surface smaller than raw ESP-IDF
- support one shared platform across `esp32`, `esp32c3`, and `esp32s3`
- keep target-specific and ESP-IDF-specific details behind internal boundaries
- give product code a clear answer to "where does this belong?"

**Success means:** examples build on all supported targets, the layering rules are obvious, and product code does not need to understand low-level ESP-IDF details just to get started.

**Non-goals:** mirroring every ESP-IDF feature, exposing internal HAL details publicly, or adding board-specific product helpers to the core platform.

## Three-Tier Model

```text
Product / example app
  → Framework API           (components/blusys_framework/include/blusys/framework/...)
  → Services API            (components/blusys_services/include/blusys/<category>/*.h)
  → HAL + drivers API       (components/blusys_hal/include/blusys/*.h,
                              components/blusys_hal/include/blusys/drivers/<category>/*.h)
  → HAL + drivers impl      (components/blusys_hal/src/soc/*.c,
                              components/blusys_hal/src/platform/*.c,
                              components/blusys_hal/src/usb/*.c,
                              components/blusys_hal/src/wireless/*.c,
                              components/blusys_hal/src/ulp/*.c,
                              components/blusys_hal/src/rt/*.c,
                              components/blusys_hal/src/drivers/<category>/*.c)
  → Internal helpers        (components/blusys_hal/include/blusys/internal/)
  → Target capability data  (components/blusys_hal/src/targets/esp32*/target_caps.c)
  → ESP-IDF
```

Dependency direction is one-way:

```text
blusys_framework → blusys_services → blusys_hal
```

Reverse dependencies are forbidden.

## Tier Details

### HAL + Drivers (`components/blusys_hal/`)

This component now contains two internal layers.

**HAL modules**
- foundational: `version`, `error`, `target`, `system`, `efuse`
- retained low-power task engine: `ulp`
- stateless pin API: `gpio`
- handle-based master/TX APIs: `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`, `rmt`, `i2s`, `twai`, `pcnt`, `dac`, `sdm`, `mcpwm`, `touch`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `usb_host`, `usb_device`
- handle-based slave/RX counterparts: `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx`
- storage: `nvs`, `sd_spi`
- external bus-backed peripherals: `one_wire`, `gpio_expander`

**Driver modules**
- display: `lcd`, `led_strip`, `seven_seg`
- input: `button`, `encoder`
- sensor: `dht`
- actuator: `buzzer`

Directory split:
- HAL implementation: `components/blusys_hal/src/soc/` (SoC peripherals), `src/platform/` (platform primitives), `src/usb/`, `src/wireless/`, `src/ulp/`, `src/rt/` (RTOS glue)
- driver implementation: `components/blusys_hal/src/drivers/<category>/`
- HAL public headers: `components/blusys_hal/include/blusys/*.h`
- driver public headers: `components/blusys_hal/include/blusys/drivers/<category>/*.h`

Umbrella header:

```c
#include "blusys/blusys.h"
```

### Services (`components/blusys_services/`)

Services are stateful runtime modules built on HAL + drivers.

- display/runtime: `ui`
- input/runtime: `usb_hid`
- connectivity: `wifi`, `wifi_prov`, `wifi_mesh`, `espnow`, `bluetooth`, `ble_gatt`, `mdns`
- protocol: `mqtt`, `http_client`, `http_server`, `ws_client`
- system: `fs`, `fatfs`, `console`, `power_mgmt`, `sntp`, `ota`, `local_ctrl`

`ui` stays here because it owns LVGL lifecycle, render-task behavior, and display flush orchestration. `usb_hid` stays here because it spans USB-host and BLE runtime behavior rather than acting as a simple hardware driver.

Umbrella header:

```c
#include "blusys/blusys_services.h"
```

### Framework (`components/blusys_framework/`)

The framework tier is the only C++ component, with `-fno-exceptions -fno-rtti` and a fixed-capacity allocation policy. It ships two layers: the product-facing `blusys::app` API and the internal framework spine that powers it.

#### Product layer — `blusys::app` (`include/blusys/app/`)

This is the recommended product-facing API. Normal product code only touches this layer.

- `app.hpp` — umbrella include for the product API
- `app_spec.hpp` — `app_spec<State, Action>` template: initial state, `update()` reducer, lifecycle hooks (`on_init`, `on_tick`), intent and event bridges, capability config, theme
- `app_ctx.hpp` — `app_ctx`: dispatch actions, emit feedback, query capability status, `product_state`, connectivity helpers; **`services()`** returns `app_services &` for routing/UI/FS
- `app_services.hpp` — `app_services`: navigate (`navigate_to`, `navigate_push`, `navigate_back`), show/hide overlays, `screen_router` / `shell` / stack helpers, ESP `spiffs` / `fatfs` when applicable
- `capability_list.hpp` — fixed array of capability pointers for `app_spec` (included from `app.hpp`; used in `integration/` for wiring)
- `entry.hpp` — entry macros: `BLUSYS_APP_MAIN_HOST(spec)`, `BLUSYS_APP_MAIN_HEADLESS(spec)`, `BLUSYS_APP_MAIN_DEVICE(spec, profile)`
- `detail/app_runtime.hpp` — runtime engine (internal, driven by the entry macros)

**View layer** (`include/blusys/app/view/`, gated by `BLUSYS_BUILD_UI`):
- `view.hpp` — view definition and lifecycle
- `bindings.hpp` — reactive data bindings for text, value, enabled, visible
- `action_widgets.hpp` — pre-built action-dispatching widgets
- `page.hpp` — page and screen helpers
- `overlay_manager.hpp` — overlay registration and control
- `custom_widget.hpp` — formal custom widget contract
- `lvgl_scope.hpp` — bounded raw LVGL access blocks

**Platform profiles** (`include/blusys/app/profiles/`):
- `host.hpp` — SDL2 host simulator profile
- `headless.hpp` — no-display marker profile
- `st7735.hpp` — generic SPI ST7735 TFT profile (ESP32, ESP32-C3, ESP32-S3)

**Capabilities** (`include/blusys/app/capabilities/`):
- `connectivity.hpp` — Wi-Fi, SNTP, mDNS, local control lifecycle
- `storage.hpp` — SPIFFS and FAT filesystem mounting

#### Internal spine (`include/blusys/framework/core/`)

The core spine is internal framework machinery. Product code does not interact with it directly — the `blusys::app` layer wraps it.

- `router.hpp` — six route commands (`set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`) plus a `route_sink` interface; interactive products use `blusys::app::view::screen_router` as the bound sink so stack navigation and screen lifecycle stay framework-owned
- `intent.hpp` — nine semantic intents (`press`/`long_press`/`release`/`confirm`/`cancel`/`increment`/`decrement`/`focus_next`/`focus_prev`) and the `app_event` envelope
- `feedback.hpp` — three channels (`visual`/`audio`/`haptic`), six patterns, fixed-capacity bus
- `runtime.hpp` — event ring buffer, optional tick cadence, `runtime_handler` callbacks (`on_init` / `handle_event` / `on_tick`); routes go directly to the bound `route_sink` (no internal route queue). `blusys::app::detail::app_runtime` wires this to the product reducer
- `containers.hpp` — `static_vector`, `ring_buffer`, `array`, `string`

#### Widget kit (`include/blusys/framework/ui/`, gated by `BLUSYS_BUILD_UI`)

- `theme.hpp` — single `theme_tokens` struct populated at boot, `set_theme()` / `theme()` accessors
- `callbacks.hpp` — semantic callback types (`press_cb_t`, `toggle_cb_t`, `change_cb_t`, ...) — products never see `lv_event_cb_t`
- `primitives.hpp` — umbrella over layout primitives (`screen`, `row`, `col`, `label`, `divider`, `icon_label`, `status_badge`, `key_value`, …)
- `widgets.hpp` — umbrella over primitives plus stock widgets (`bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`, `bu_progress`, `bu_list`, `bu_card`, `bu_tabs`, `bu_dropdown`, `bu_gauge`, `bu_knob`, `bu_chart`, `bu_data_table`, `bu_input_field`, `bu_level_bar`, `bu_vu_strip`, …)
- Layout primitives: `screen`, `row`, `col`, `label`, `divider`, `icon_label`, `status_badge`, `key_value`
- Stock interactive widgets: `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay`, `bu_list`, `bu_tabs`, `bu_dropdown`, `bu_input_field`, `bu_knob`
- Stock display widgets: `bu_progress`, `bu_chart`, `bu_gauge`, `bu_data_table`, `bu_card`, `bu_level_bar`, `bu_vu_strip`
- `input/encoder.hpp` — `create_encoder_group` + `auto_focus_screen` for encoder-driven focus traversal

Authoring contract: every widget follows the six-rule contract (theme tokens only, config struct interface, setters own state transitions, standard state set, one folder per widget, header is the spec). Stock-backed widgets use a fixed-capacity slot pool keyed by `BLUSYS_UI_<NAME>_POOL_SIZE` for callback storage. See `components/blusys_framework/widget-author-guide.md`.

**UI layout (LVGL flex):** see [UI layout](#ui-layout-lvgl-flex) below. Shared helpers for flex children live in `components/blusys_framework/include/blusys/framework/ui/detail/flex_layout.hpp` (included from `primitives.hpp`).

End-to-end validation: `examples/reference/framework_app_basic/` exercises the full framework validation chain. `examples/reference/connected_headless/` validates the headless path with capabilities and the reducer model; `examples/reference/connected_device/` does the same for interactive + connectivity. Device-profile validation lives under `examples/reference/framework_device_basic/`.

Host iteration: `scripts/host/` builds LVGL against SDL2 on Linux via `blusys host-build`, pinned to the same upstream LVGL tag as the ESP-IDF managed component. Lets the widget kit be iterated on without flashing hardware on every change.

## Layering Rules

- public headers are application-facing
- internal code may depend on ESP-IDF details
- target-specific behavior stays behind internal boundaries
- services never depend on framework
- HAL code must not include driver headers

The HAL/drivers boundary is enforced by `blusys lint`, backed by `scripts/lint-layering.sh`.

The lint checks exactly two rules:
- no file under `components/blusys_hal/src/soc/`, `src/platform/`, `src/usb/`, or `src/wireless/` may include `blusys/drivers/**`
- files under `components/blusys_hal/src/drivers/` may only include the shared internal allowlist: `blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h`, and (display stack) `lcd_panel_ili.h`

## Symmetric Pairs

Some HAL modules expose the same peripheral in both directions under separate handles:

- I2C: `blusys_i2c_master_*` and `blusys_i2c_slave_*`
- SPI: `blusys_spi_*` and `blusys_spi_slave_*`
- I2S: `blusys_i2s_tx_*` and `blusys_i2s_rx_*`
- RMT: `blusys_rmt_*` and `blusys_rmt_rx_*`

## CMake Usage

HAL-only or driver examples:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys_hal)
```

Service examples:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys_services)
```

Framework examples (and the bootstrap entry of scaffolded product apps):

```cmake
idf_component_register(SRCS "main.cpp" REQUIRES blusys_framework)
```

Two consumption models, deliberately separated:

- **Bundled examples** in `examples/` are discovered via
  `EXTRA_COMPONENT_DIRS` (typically `${BLUSYS_REPO_ROOT}/components`) in each
  example’s top-level `CMakeLists.txt`. This pattern is for in-repo builds where
  the platform components live next door.
- **Scaffolded product apps** from `blusys create [--interface …] [--with …] [--policy …]`
  use the same mechanism: the generated top-level `CMakeLists.txt` embeds
  `EXTRA_COMPONENT_DIRS` pointing at the `components/` tree from the checkout
  used at generation time (plus `main/idf_component.yml` for ESP-IDF and managed
  deps such as LVGL). Vendor or repoint `EXTRA_COMPONENT_DIRS` for standalone
  trees. Product code stays under `main/core/`, `main/ui/` (when interactive),
  and `main/integration/`. See [Product shape](../start/product-shape.md) and
  [Interactive Quickstart](../start/quickstart-interactive.md); [App](../app/index.md)
  covers the product model.

## Design Rationale

Key decisions and their reasoning, preserved here for future contributors.

**HAL and drivers stay C.** HAL is the layer closest to the MCU. Explicit lifecycle,
deterministic behavior, and the simplest possible low-level surface are most critical
here. HAL wraps ESP-IDF drivers that are themselves C. Drivers (display, input, sensor,
actuator) compose HAL primitives into hardware drivers; they are mostly stateless or hold
trivial state and do not benefit meaningfully from C++ encapsulation.

**Drivers live inside `components/blusys_hal/`, not a fourth component.** The HAL/drivers
boundary is enforced by directory discipline and CI lint (not a component boundary)
because the operational boundary is about what the code *does*, not where it lives. A
fourth component would add a `REQUIRES` edge with no meaningful encapsulation benefit.

**Services stay C in V1.** The existing C service implementations (`wifi.c`, `mqtt.c`,
`ota.c`) are already well-factored around opaque handles and explicit lifecycle — the
same structure C++ classes would give. Migrating in V1 would force every service example
from `.c` to `.cpp` and open a hybrid C/C++ window for speculative benefit.

**Framework is C++** because the widget kit needs designated initializers for config
structs, captureless lambdas for semantic callbacks, `enum class` for variants, and
template containers. Controller state machines, router logic, intent dispatch, and
feedback channels all benefit from the same toolset. C++ earns its keep concretely here.

**Fixed-capacity allocation.** The framework spine and widget kit never call `new` or
`malloc` after initialization. Widget callback slots use static pools sized by KConfig.
The pool fires a fail-loud assert on exhaustion — silent exhaustion is worse than a crash.
This policy keeps heap fragmentation predictable on embedded targets.

**Single version tag per release.** All three components (`blusys_hal`, `blusys_services`,
`blusys_framework`) ship under one git tag. Product repos pin one tag in their
`idf_component.yml`. A three-component version matrix would require product teams to
reason about inter-component compatibility on every update.

**Theme as one struct.** The `theme_tokens` struct is populated at boot by product code
and read by every widget. No JSON pipeline, no design-tool integration, no secondary
theme-file format. Products tune colors, radii, spacing, and fonts by filling in the
struct; widgets pick up changes automatically.

**`main/` is the single local component for scaffolded product apps.** The
canonical scaffold keeps all product-owned code under `main/core/`, `main/ui/`,
and `main/integration/` so the app structure stays obvious and the local
component model stays singular.

**Six-rule widget contract.** Every widget follows the same contract (theme tokens only,
config struct interface, setters own state transitions, standard state set, one folder,
header is the spec) so any developer can read any widget and know where to look. The
contract is enforced by code review, not tooling.

## Lifecycle Verbs

Lifecycle verbs stay explicit where relevant:

- `open`
- `close`
- `start`
- `stop`
- `read`
- `write`
- `transfer`

## Target Policy

The platform targets the common subset of `esp32`, `esp32c3`, and `esp32s3`. Modules not available on all three targets use capability checks and return `BLUSYS_ERR_NOT_SUPPORTED` on unsupported hardware. See [Compatibility](target-matrix.md) for the full matrix.

## UI layout (LVGL flex) {#ui-layout-lvgl-flex}

The framework uses LVGL’s built-in flex and scroll so product code declares structure (columns, rows, shell) without reimplementing layout rules.

**Principles:**

1. **Prefer LVGL flex** (`LV_LAYOUT_FLEX`, `lv_obj_set_flex_flow`, `lv_obj_set_flex_align`). No parallel layout engine.
2. **Bound the scroll viewport.** The shell’s scrollable page column must be a flex child with bounded height (`shell` `content_area` + `page_create_in`) so chrome (tabs, header) is not pushed off-screen.
3. **Stock widgets size to constraints.** Widgets placed in flex rows with a fixed row height must respect the allocated size (`bu_gauge` scales its arc on `LV_EVENT_SIZE_CHANGED` via `blusys::ui::flex_layout::effective_cross_extent_for_row_child`).
4. **Escape hatch.** Custom product UI may use raw LVGL inside custom widgets or an explicit custom scope, keeping the same focus/action/runtime contracts as `widget-author-guide.md`.

**Primitives:**

- `col` / `row` — flex column/row helpers. `row_config` / `col_config` expose main/cross/track flex align (`row` defaults: `START`, `CENTER`, `CENTER`; `col`: `START`, `START`, `START`).
- `blusys::ui::flex_layout` — helpers for flex row/column parents (`effective_cross_extent_for_row_child`, `effective_cross_extent_for_column_child`) when a child’s content-sized dimension under-reports the strip.
- `page_create` / `page_create_in` — page column; the in-shell variant sets `flex_grow(1)`, `min_height(0)`, and optional scroll so the column fills `content_area` correctly.

**Shell:** column flex root — header, optional status, `content_area` (`flex_grow(1)`), optional tab bar. `content_area` is itself a column flex container so its single child (the scrollable page column) participates as a proper flex item.

## Display notes: ST7735 on SPI

`blusys_st7735_panel_draw_bitmap()` in `components/blusys_hal/src/drivers/display/lcd.c` sends **one display row per `RAMWR`** (each row: `COLMOD` + `CASET` + `RASET` + `RAMWR`). Packing several scanlines into one `RASET` window plus one long `RAMWR` caused diagonal shear on some ST7735 SPI modules; the per-row path fixes it. `COLMOD` is re-asserted per transaction and once more after the last `RAMWR` so DMA completion is synchronized before the source buffer is reused. Throughput is lower than multi-row bursts; override only with validated hardware.

LVGL’s partial-mode `flush_cb` in `components/blusys_services/src/display/ui.c` copies each dirty region from `px_map` using `lv_draw_buf_width_to_stride(area_width, cf)`, packs into the DMA scratch buffer, optionally byte-swaps RGB565 for SPI, and calls `blusys_lcd_draw_bitmap()`. `lv_display_flush_ready()` runs after all SPI/DMA for that flush completes.

SPI default for `st7735_160x128()` is 16 MHz; lower `pclk_hz` in product integration if wiring is marginal. Typical x/y gap offsets for 160×128 modules remain in the profile; tune per glass as needed.

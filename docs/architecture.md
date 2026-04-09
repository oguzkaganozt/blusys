# Architecture

Blusys is an internal ESP32 product platform. The repo ships three ESP-IDF components
sharing the `blusys/` header namespace:

```text
components/blusys/            HAL + drivers
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
  → HAL + drivers API       (components/blusys/include/blusys/*.h,
                              components/blusys/include/blusys/drivers/<category>/*.h)
  → HAL + drivers impl      (components/blusys/src/common/*.c,
                              components/blusys/src/drivers/<category>/*.c)
  → Internal helpers        (components/blusys/include/blusys/internal/)
  → Target capability data  (components/blusys/src/targets/esp32*/target_caps.c)
  → ESP-IDF
```

Dependency direction is one-way:

```text
blusys_framework → blusys_services → blusys
```

Reverse dependencies are forbidden.

## Tier Details

### HAL + Drivers (`components/blusys/`)

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
- HAL implementation: `components/blusys/src/common/`
- driver implementation: `components/blusys/src/drivers/<category>/`
- HAL public headers: `components/blusys/include/blusys/*.h`
- driver public headers: `components/blusys/include/blusys/drivers/<category>/*.h`

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

The framework tier is the only C++ component in V1, with `-fno-exceptions -fno-rtti` and a fixed-capacity allocation policy. It ships the V1 product surface in full:

**Core spine** (`include/blusys/framework/core/`):
- `router.hpp` — six route commands (`set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`) plus a `route_sink` interface
- `intent.hpp` — nine semantic intents (`press`/`long_press`/`release`/`confirm`/`cancel`/`increment`/`decrement`/`focus_next`/`focus_prev`) and the `app_event` envelope
- `feedback.hpp` — three channels (`visual`/`audio`/`haptic`), six patterns, fixed-capacity bus
- `controller.hpp` — `init`/`handle`/`tick`/`deinit` lifecycle base class with `submit_route` and `emit_feedback` helpers
- `runtime.hpp` — event/route ring buffers, 10 ms default tick cadence, `route_sink` and `feedback_bus` integration
- `containers.hpp` — `static_vector`, `ring_buffer`, `array`, `string`

**UI layer** (`include/blusys/framework/ui/`, gated by `BLUSYS_BUILD_UI`):
- `theme.hpp` — single `theme_tokens` struct populated at boot, `set_theme()` / `theme()` accessors
- `callbacks.hpp` — semantic callback types (`press_cb_t`, `toggle_cb_t`, `change_cb_t`, ...) — products never see `lv_event_cb_t`
- `widgets.hpp` — umbrella over all widget kit headers
- Layout primitives: `screen`, `row`, `col`, `label`, `divider`
- V1 widget kit: `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`, `bu_overlay` (`bu_knob` deferred to V2)
- `input/encoder.hpp` — `create_encoder_group` + `auto_focus_screen` for encoder-driven focus traversal

Authoring contract: every widget follows the six-rule contract (theme tokens only, config struct interface, setters own state transitions, standard state set, one folder per widget, header is the spec). Camp 2 stock-backed widgets use a fixed-capacity slot pool keyed by `BLUSYS_UI_<NAME>_POOL_SIZE` for callback storage. See `components/blusys_framework/widget-author-guide.md`.

End-to-end validation: `examples/framework_app_basic/` exercises the full chain (`button.on_press` → `runtime.post_intent` → `controller.handle` → `slider_set_value` / `submit_route` → `ui_route_sink` → `overlay_show`).

Host iteration: `scripts/host/` builds LVGL against SDL2 on Linux via `blusys host-build`, pinned to the same upstream LVGL tag as the ESP-IDF managed component. Lets the widget kit be iterated on without flashing hardware on every change.

## Layering Rules

- public headers are application-facing
- internal code may depend on ESP-IDF details
- target-specific behavior stays behind internal boundaries
- services never depend on framework
- HAL/common code must not include driver headers

The HAL/drivers boundary is enforced by `blusys lint`, backed by `scripts/lint-layering.sh`.

The lint checks exactly two rules:
- no file under `components/blusys/src/common/` may include `blusys/drivers/**`
- files under `components/blusys/src/drivers/` may only include the shared internal allowlist: `blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h`

## Symmetric Pairs

Some HAL modules expose the same peripheral in both directions under separate handles:

- I2C: `blusys_i2c_master_*` and `blusys_i2c_slave_*`
- SPI: `blusys_spi_*` and `blusys_spi_slave_*`
- I2S: `blusys_i2s_tx_*` and `blusys_i2s_rx_*`
- RMT: `blusys_rmt_*` and `blusys_rmt_rx_*`

## CMake Usage

HAL-only or driver examples:

```cmake
idf_component_register(SRCS "main.c" REQUIRES blusys)
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
  `EXTRA_COMPONENT_DIRS ../../components` in each example project's top-level
  `CMakeLists.txt`. This pattern is **only** valid for the in-repo examples,
  where the platform components live next door.
- **Scaffolded product apps** generated by `blusys create [--starter ...]`
  pull all three platform components through ESP-IDF's managed component
  manager from `main/idf_component.yml`. The scaffolded top-level
  `CMakeLists.txt` scopes `EXTRA_COMPONENT_DIRS` to `${CMAKE_CURRENT_LIST_DIR}/app`
  so ESP-IDF discovers the local `app/` component without re-including the
  project root. See [Getting Started](guides/getting-started.md) for the
  scaffold layout and the [Framework guide](guides/framework.md#scaffolding-a-new-product)
  for the starter-type semantics.

## Design Rationale

Key decisions and their reasoning, preserved here for future contributors.

**HAL and drivers stay C.** HAL is the layer closest to the MCU. Explicit lifecycle,
deterministic behavior, and the simplest possible low-level surface are most critical
here. HAL wraps ESP-IDF drivers that are themselves C. Drivers (display, input, sensor,
actuator) compose HAL primitives into hardware drivers; they are mostly stateless or hold
trivial state and do not benefit meaningfully from C++ encapsulation.

**Drivers live inside `components/blusys/`, not a fourth component.** The HAL/drivers
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

**Single version tag per release.** All three components (`blusys`, `blusys_services`,
`blusys_framework`) ship under one git tag. Product repos pin one tag in their
`idf_component.yml`. A three-component version matrix would require product teams to
reason about inter-component compatibility on every update.

**Theme as one struct.** The `theme_tokens` struct is populated at boot by product code
and read by every widget. No JSON pipeline, no design-tool integration, no secondary
theme-file format. Products tune colors, radii, spacing, and fonts by filling in the
struct; widgets pick up changes automatically.

**`EXTRA_COMPONENT_DIRS` points at `app/`, not the project root.** ESP-IDF's
`__project_component_dir` helper treats any path containing a `CMakeLists.txt` as a
component itself. Pointing at the project root would recursively include the top-level
CMakeLists during requirement scanning. Pointing at `app/` registers exactly one local
component.

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

# Migration Guide — V5 → V6

This guide walks product repos from the V5 two-component model (released as
`v5.0.0`) to the V6 three-tier model landed during the platform transition.

The changes are **mostly mechanical**: rename a few headers, move driver
`REQUIRES` between components, bump the pinned version in
`main/idf_component.yml`. If your product has an existing `app_main.c` that
only uses HAL modules and services, the rename pass below is enough and you
do not need to touch the rest of the guide.

If you are starting a new V6 product, don't follow this guide — run
`blusys create --starter <headless|interactive>` instead. That scaffolds
the four-CMakeLists layout, `main/app_main.cpp`, and a pinned
`main/idf_component.yml` in one step.

## TL;DR — the absolute minimum

For a product that only uses HAL modules and services and does **not** adopt
the framework:

1. Drop `#include "blusys/blusys_all.h"` and replace with one (or both) of
   `#include "blusys/blusys.h"` and `#include "blusys/blusys_services.h"`.
2. Change driver-using examples from `REQUIRES blusys_services` to
   `REQUIRES blusys` for `lcd`, `led_strip`, `seven_seg`, `button`,
   `encoder`, `dht`, and `buzzer`.
3. Bump your pinned version in `main/idf_component.yml` to `v6.0.0`.

Everything else is optional: adopting the framework, migrating to the
scaffold layout, switching `main/` to C++, and so on.

## What changed between V5 and V6

### 1. Component shape: two tiers → three tiers

| Tier          | V5 location                             | V6 location                                |
|---------------|-----------------------------------------|--------------------------------------------|
| HAL + drivers | `components/blusys/` (HAL only)         | `components/blusys/` (HAL **and** drivers) |
| Services      | `components/blusys_services/` (incl. drivers) | `components/blusys_services/` (no drivers) |
| Framework     | (did not exist)                         | `components/blusys_framework/` (new, C++)  |

Rationale and full details: [`platform-transition/ROADMAP.md`](https://github.com/oguzkaganozt/blusys/blob/main/platform-transition/ROADMAP.md)
and [`platform-transition/DECISIONS.md`](https://github.com/oguzkaganozt/blusys/blob/main/platform-transition/DECISIONS.md).

### 2. Drivers moved from `blusys_services` into `blusys`

The following seven driver modules moved from
`components/blusys_services/src/<category>/` to
`components/blusys/src/drivers/<category>/` in Phase 3 of the transition:

| Module      | V5 location                             | V6 location                                      |
|-------------|------------------------------------------|--------------------------------------------------|
| `lcd`       | `blusys_services/src/display/lcd.c`      | `blusys/src/drivers/display/lcd.c`               |
| `led_strip` | `blusys_services/src/display/led_strip.c`| `blusys/src/drivers/display/led_strip.c`         |
| `seven_seg` | `blusys_services/src/display/seven_seg.c`| `blusys/src/drivers/display/seven_seg.c`         |
| `button`    | `blusys_services/src/input/button.c`     | `blusys/src/drivers/input/button.c`              |
| `encoder`   | `blusys_services/src/input/encoder.c`    | `blusys/src/drivers/input/encoder.c`             |
| `dht`       | `blusys_services/src/sensor/dht.c`       | `blusys/src/drivers/sensor/dht.c`                |
| `buzzer`    | `blusys_services/src/actuator/buzzer.c`  | `blusys/src/drivers/actuator/buzzer.c`           |

The **public header paths did not change** — `blusys/drivers/<category>/<module>.h`
is a new umbrella location but the old `blusys/<category>/<module>.h` aliases
still compile. The practical effect for product code is that consumers need
to switch their component `REQUIRES` line from `blusys_services` to `blusys`
for any of the seven drivers above.

`usb_hid` **stayed in services** — it is runtime orchestration across USB host
and BLE, not a simple driver. No change required for `usb_hid` users.

### 3. `blusys/blusys_all.h` removed

The catch-all umbrella header was deleted in Phase 8. Code that used it must
switch to the per-tier umbrella headers:

```c
// V5
#include "blusys/blusys_all.h"

// V6
#include "blusys/blusys.h"            // HAL + drivers (C)
#include "blusys/blusys_services.h"   // services (C)
```

For framework-using code (new in V6), add:

```cpp
#include "blusys/framework/framework.hpp"
```

If your file only uses one driver, prefer the module-specific header for
faster builds:

```c
#include "blusys/drivers/display/lcd.h"
#include "blusys/drivers/input/button.h"
```

### 4. New: `blusys_framework` (C++20, optional)

V6 adds a third tier at `components/blusys_framework/`. It is **C++20 only**
(`-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics`) and ships:

- **Core spine** — `runtime`, `controller`, `intent`, `feedback`, `router`.
- **V1 widget kit** — `bu_button`, `bu_toggle`, `bu_slider`, `bu_modal`,
  `bu_overlay` (LVGL-backed, slot-pool callback storage).
- **Layout primitives** — `screen`, `row`, `col`, `label`, `divider`.
- **Encoder helpers** — `create_encoder_group`, `auto_focus_screen`.
- **Theme registry** — single C++ struct populated at boot.

The framework is **optional**. Headless products (MQTT gateways, sensors
with no LCD) skip it entirely. Interactive products (panels, meters,
remotes) opt in by setting `BLUSYS_STARTER_TYPE=interactive` in
`app/product_config.cmake`, which turns on `BLUSYS_BUILD_UI` and pulls
LVGL in as a managed component.

Full surface reference: [`App`](../app/index.md) (supersedes the v6 framework guide).

### 5. Scaffold layout and `idf_component.yml` consumption

V5 examples used `EXTRA_COMPONENT_DIRS=$BLUSYS_PATH/components` for monorepo
consumption. That pattern still works for **examples inside this repo**, but
V6 product repos consume the platform via **managed components** in
`main/idf_component.yml` pinned to a release tag. Don't mix the two.

The scaffold generated by `blusys create` builds a four-CMakeLists
product layout:

```
my_product/
├── CMakeLists.txt          # top-level project, sets BLUSYS_BUILD_UI
├── main/
│   ├── CMakeLists.txt      # main component, REQUIRES blusys{,_services,_framework}
│   ├── app_main.cpp        # always .cpp; no blusys_all.h
│   └── idf_component.yml   # pinned platform + lvgl versions
└── app/
    ├── CMakeLists.txt      # app component (product code)
    ├── product_config.cmake # BLUSYS_STARTER_TYPE, targets, product name
    ├── controllers/home_controller.{hpp,cpp}
    └── (interactive only) ui/theme_init, ui/screens/main_screen
```

### 6. Logging surface

Framework code uses `#include "blusys/log.h"` + `BLUSYS_LOGE/W/I/D` macros.
HAL + drivers + services continue to use `esp_log.h` + `ESP_LOGE/W/I/D`
directly. Don't cross the streams — `blusys/log.h` is a thin wrapper that
exists so the framework surface can be re-pointed at a non-ESP logger in
future without touching every widget.

### 7. ESP-IDF baseline

V6 requires **ESP-IDF v5.5.4** or newer. V5 supported v5.1.x and v5.2.x.
If your product pins an older IDF in `main/idf_component.yml`, bump it:

```yaml
dependencies:
  idf:
    version: ">=5.5"
```

## Scenario 1 — existing product, no framework adoption

Your product uses HAL + services and does not need the widget kit or
runtime. This is the most common migration and is almost entirely
mechanical.

**Step 1.** Rewrite any `#include "blusys/blusys_all.h"` lines:

```c
#include "blusys/blusys.h"
#include "blusys/blusys_services.h"
```

**Step 2.** In `main/CMakeLists.txt`, switch `REQUIRES blusys_services` to
`REQUIRES blusys` for any of the seven driver modules listed in §2 above.
If your product uses both drivers and services, list both:

```cmake
idf_component_register(
    SRCS "app_main.c"
    REQUIRES blusys blusys_services
)
```

**Step 3.** In `main/idf_component.yml`, bump the pinned version. The V5
line was typically:

```yaml
dependencies:
  blusys_services:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys_services"
```

V6 needs both `blusys` **and** `blusys_services` pinned to `v6.0.0`:

```yaml
dependencies:
  idf:
    version: ">=5.5"

  blusys:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "v6.0.0"
    path: "components/blusys"
  blusys_services:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "v6.0.0"
    path: "components/blusys_services"
```

**Step 4.** Delete any cached managed-components state and rebuild:

```bash
rm -rf managed_components build-esp32s3
blusys build . esp32s3
```

That's it. Your `app_main.c` does not need to become C++, and the framework
tier is not built into your firmware.

## Scenario 2 — existing product, adopting the framework

You have an existing V5 product and want to bring in the framework runtime
and widget kit. Two options:

### Option A (recommended) — regenerate with the scaffold and port code in

1. Scaffold a fresh product: `blusys create --starter <headless|interactive> my_product_v6`
2. Delete the sample `home_controller` + `main_screen` + `theme_init`.
3. Copy your existing `app_main` body into `my_product_v6/main/app_main.cpp`.
   You'll need to rename it from `.c` to `.cpp` and (if it was
   `void app_main(void)` in C) wrap it in `extern "C"`.
4. Copy your existing product code into `my_product_v6/app/` as the `app/`
   ESP-IDF component.
5. Point `main/idf_component.yml` at `v6.0.0`.

This gets you onto the V6 layout in one pass and is the path the scaffold
was designed for.

### Option B — incremental adoption

Keep your existing layout and pull in only what you need.

1. Complete Scenario 1 first (headers, `REQUIRES`, version bump).
2. In `main/CMakeLists.txt`, add `blusys_framework` to `REQUIRES` and
   rename your main file from `.c` to `.cpp`:

   ```cmake
   idf_component_register(
       SRCS "app_main.cpp"
       REQUIRES blusys blusys_services blusys_framework
   )
   ```

3. In `main/idf_component.yml`, add the framework component pinned to
   `v6.0.0`. If you want the widget kit, also add `lvgl/lvgl ~9.2`.
4. Set `BLUSYS_BUILD_UI=ON` in your top-level `CMakeLists.txt` before the
   `project(...)` call if you are opting into the UI tier:

   ```cmake
   set(BLUSYS_BUILD_UI ON CACHE BOOL "build framework UI tier")
   include($ENV{IDF_PATH}/tools/cmake/project.cmake)
   project(my_product)
   ```

5. In your new `app_main.cpp`, call `blusys::framework::init()` and wire
   up a `controller` subclass + `runtime.init(...)`. The minimum wiring
   is in [`App`](../app/index.md) and in
   `examples/framework_core_basic/` (headless) or
   `examples/framework_app_basic/` (full spine + widgets).

Option B is more disruptive to your build files but preserves your
existing commit history and any product-specific `CMakeLists.txt`
customisations.

## Scenario 3 — upgrading a product between V6 releases

For minor / patch bumps after `v6.0.0`, the process is just:

```yaml
dependencies:
  blusys:
    version: "v6.x.y"
  blusys_services:
    version: "v6.x.y"
  blusys_framework:
    version: "v6.x.y"
```

**All three tiers must be pinned to the same tag.** Mixing tiers across
versions is not supported — see decision 7 in
[`platform-transition/DECISIONS.md`](https://github.com/oguzkaganozt/blusys/blob/main/platform-transition/DECISIONS.md).
The framework's ABI is not stable between minor versions, and the
framework's internal use of services assumes matching versions.

After editing `idf_component.yml`:

```bash
rm -rf managed_components build-*
blusys build . esp32s3
```

Check the changelog (GitHub release notes) before upgrading — V6 minor
releases may expand the widget kit or rename framework symbols.

## Scenario 4 — adding a new capability to an existing V6 product

Most new capability work does not touch the platform at all — you add a
new controller or new screen in `app/` and that's it. The platform only
needs touching when the capability needs:

- **A new HAL or service module** that does not exist upstream yet. Follow
  "Adding a New Module" in [`CLAUDE.md`](https://github.com/oguzkaganozt/blusys/blob/main/CLAUDE.md) and open a PR
  against the blusys repo. Consume it via an `override_path` in your
  product's `idf_component.yml` until the upstream change is released.
- **A new managed component** (e.g. `espressif/esp_camera`). Add it to
  your product's `main/idf_component.yml`; if the service-tier code needs
  to adapt to it, do so behind an `if(... IN_LIST build_components)`
  check the way `mdns` / `ui` / `usb_hid` already do.
- **A new widget.** For V1, open a PR against `blusys_framework` following
  [`components/blusys_framework/widget-author-guide.md`](https://github.com/oguzkaganozt/blusys/blob/main/components/blusys_framework/widget-author-guide.md).
  V2 will allow product-repo widgets; V1 keeps the kit in the platform so
  the six-rule contract can be enforced centrally.

For anything else — new controllers, new screens, new feedback sinks,
new route sinks, new integrations posting `app_event`s into the
runtime — stay in `app/` and the platform is untouched.

## Troubleshooting

**`fatal error: blusys/blusys_all.h: No such file or directory`** — the
umbrella was removed in Phase 8. Replace with the per-tier umbrella
headers (see §3).

**`undefined reference to blusys_button_open`** — your `main/CMakeLists.txt`
is `REQUIRES blusys_services` but `button` moved to `blusys`. Add
`blusys` to the `REQUIRES` list.

**`error: 'blusys::framework' has not been declared`** — either your
file is still `.c` and needs to be renamed to `.cpp`, or
`blusys_framework` is not in your component `REQUIRES`.

**Build pulls in both `override_path` and `version` for the same dep** —
`override_path` takes precedence. Remove the `git`/`version`/`path`
lines for any dep you are overriding locally, or the managed-components
tool may complain about redundant fields.

**Two `CMakeLists.txt` files found under the `EXTRA_COMPONENT_DIRS`
target** — your top-level `CMakeLists.txt` is pointing at the project
root instead of `${CMAKE_CURRENT_LIST_DIR}/app`. See the scaffold spec
correction in [`platform-transition/DECISIONS.md`](https://github.com/oguzkaganozt/blusys/blob/main/platform-transition/DECISIONS.md)
(decision 15 amendment) and Phase 7 notes.

**`BLUSYS_BUILD_UI=ON requires the lvgl/lvgl managed component`** —
you've enabled the UI tier but did not add `lvgl/lvgl ~9.2` to
`main/idf_component.yml`. Either add it, or set `BLUSYS_BUILD_UI=OFF`
for a headless product.

## Related references

- [Architecture](../internals/architecture.md) — the current three-tier layout.
- [App](../app/index.md) — v7 product-facing API (supersedes the v6 framework guide).
- [Platform transition docs](https://github.com/oguzkaganozt/blusys/tree/main/platform-transition) — full planning
  history, decisions log, and phase-by-phase rationale.
- [`PROGRESS.md`](https://github.com/oguzkaganozt/blusys/blob/main/PROGRESS.md) — current transition phase and verification snapshot.
- [`CLAUDE.md`](https://github.com/oguzkaganozt/blusys/blob/main/CLAUDE.md) — contributor guidance and "Adding a New Module" workflow.

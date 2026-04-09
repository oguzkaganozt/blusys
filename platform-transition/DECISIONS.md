# Platform Transition Decisions

This document is the canonical log of architectural decisions locked during the Blusys platform transition (library → internal embedded product platform). Each decision is normative. When the surrounding docs (ROADMAP, application-layer-architecture, CPP-transition, UI-system) contradict this file, this file wins.

All decisions below are **Locked** unless otherwise noted. Decisions may be revisited if real-world pilot data contradicts the rationale, but not casually.

---

## 1. Public ABI language and C++ standard

**Decision:** HAL, drivers, and services all expose C headers (`extern "C"`). Framework is the **only C++ tier** in V1: it exposes C++ headers (namespaces, references, method-style APIs). Examples consuming only HAL, drivers, or services stay `.c`; examples consuming the framework are `.cpp`.

The C++ standard for framework is **C++20** (`-std=c++20`). This enables:
- `std::span<T>` for caller-owned buffer APIs
- designated initializers for widget config structs
- `constexpr` and `consteval` improvements for compile-time tokens

ESP-IDF 5.5 ships with GCC 14, which has full C++20 support on both Xtensa and RISC-V targets. No new toolchain work is required.

**Rationale:** C++ earns its keep in the widget kit — designated initializers for config structs, the captureless-lambda callback idiom, `enum class` for variants, template-based fixed-capacity containers. Services callback orchestration could also benefit from C++, but the existing C services (`wifi.c`, `mqtt.c`) are already held up in CLAUDE.md as correct reference implementations, and migrating them would create a long hybrid C/C++ window, force a dual-header policy, delay LTO, and convert ~20 service-tier examples to `.cpp` — all for speculative benefit. **Framework-only C++ in V1** gives us the concrete widget-kit value without the migration cost. Services can migrate per-module in V2 when there is measured pain to resolve.

C++20 is chosen over C++17 because `std::span` and designated initializers are features the widget kit uses extensively; C++17 would either degrade the API surface or rely on GCC extensions.

---

## 2. Allocation policy

**Decision:** Default allocation strategy for framework is **fixed-capacity containers + caller-owned buffers**. No `new`/`delete`, no `std::vector`, no `std::string` members. Platform ships a small container set in `blusys_framework/core/containers.h`:

- `blusys::array<T, N>` — fixed-size array with bounds-checked access
- `blusys::string<N>` — fixed-capacity string
- `blusys::ring_buffer<T, N>` — SPSC ring buffer
- `blusys::static_vector<T, N>` — fixed-capacity vector with push_back semantics

Pool allocators only when bounded allocation is genuinely required and measured. Large/variable payloads (MQTT bodies, HTTP bodies, LCD framebuffers) are caller-owned buffers passed as `std::span<uint8_t>` or raw `(ptr, len)` pairs.

**Rationale:** Embedded memory is bounded; dynamic allocation is where determinism dies. Caller-owned buffers keep the allocation decision with the product team, which knows its budget.

---

## 3. HAL component naming

**Decision:** Keep the existing directory name `components/blusys/` for the HAL component. Do **not** rename to `components/blusys_hal/`. Docs refer to it as "the HAL component" in prose. Public include namespace `blusys/...` is unchanged.

**Rationale:** Renaming means touching 68 examples plus any downstream project — real blast radius for zero technical gain. The directory name is a detail; the layering is what matters.

---

## 4. Three-tier layering with drivers inside HAL component

**Decision:** The platform has three tiers:

| Tier | Component | Language | Contains |
|---|---|---|---|
| 1 | `blusys` | C | HAL + hardware drivers. HAL sources under `src/common/` (gpio, uart, spi, i2c, adc, pwm, timer, rmt, ...), headers at `include/blusys/<module>.h`. Driver sources under `src/drivers/<category>/` (display: lcd/led_strip/seven_seg; input: button/encoder; sensor: dht; actuator: buzzer), headers at `include/blusys/drivers/<category>/<module>.h`. |
| 2 | `blusys_services` | C | Stateful runtime (connectivity, protocol, system, runtime helpers such as `ui` and `usb_hid`): wifi, mqtt, ble_gatt, ota, http_*, ws_client, fs, sntp, power_mgmt. Stays C in V1 per decision 1. |
| 3 | `blusys_framework` | C++ | App framework (the only C++ tier): router, intent, feedback, controllers, widget kit. |

Dependency direction: `blusys_framework` → `blusys_services` → `blusys`. Reverse dependencies are forbidden.

Within `components/blusys/`, the HAL/drivers boundary is enforced by **directory discipline + CI lint**, not by component boundary:

- HAL sources live in `src/common/`; driver sources live in `src/drivers/<category>/`.
- HAL public headers live in `include/blusys/<module>.h`; driver public headers live in `include/blusys/drivers/<category>/<module>.h`.
- A CI check fails the build if any file under `src/common/` includes `blusys/drivers/**`, or if any file under `src/drivers/` includes `blusys/internal/**` except the explicit shared utilities list (`blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h`).
- The shared utilities list is closed; adding a header to it requires an explicit decision.

**Rationale:** An earlier draft of this plan split drivers into their own `blusys_drivers` component. The justification was build-time enforcement of the HAL/drivers boundary. But a separate component also means a new CMakeLists, a new REQUIRES list, a new umbrella header, a new docs section, ~10 example updates per release — real ceremony for a boundary that directory discipline plus CI lint can enforce just as well. With services staying C (decision 1), the "language boundary aligns with component boundary" argument for splitting drivers out also disappears: both HAL and drivers are C. Three tiers is the simpler shape that matches the actual enforcement need. The CI lint keeps layering honest without paying the component-split cost.

`usb_hid` is explicitly kept in `blusys_services`. Although it exposes an input-facing API, its current shape spans USB-host and BLE-HOGP transports, singleton runtime state, and BT/NVS bring-up helpers. That is service-tier orchestration, not a simple hardware driver.

This decision is reversible: if the team grows and convention drift becomes a real problem, the drivers split can be done later by moving files — roughly one day of mechanical work.

---

## 5. `blusys_framework` V1 scope

**Decision:** V1 ships the **minimal spine** plus the **widget kit**. Nothing else.

`blusys_framework/core` is mandatory for both scaffold profiles, including headless products. Only `blusys_framework/ui` is optional.

Minimal spine (`blusys_framework/core`):
1. Router contract with the 6-command set (`set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`) + `route_command` payload struct
2. Intent enum + dispatch (`press`, `confirm`, `increment`, `decrement`, `focus_next`, `focus_prev`, `long_press`, `cancel`)
3. Feedback channel with pluggable sinks (visual / audio / haptic)
4. Controller lifecycle base (`init` / `handle` / `tick` / `deinit`)
5. Fixed-capacity container set (from decision 2)

Widget kit (`blusys_framework/ui`):
6. `theme_tokens` struct + `set_theme()` / `theme()` accessors
7. Layout primitives (`bu_screen`, `bu_row`, `bu_col`, `bu_stack`, `bu_spacer`, `bu_divider`, `bu_label`, `bu_text_value`) — composition factories
8. Interactive widgets (`bu_button`, `bu_slider`, `bu_toggle`, `bu_modal`, `bu_overlay`, optionally `bu_knob`) — per-widget Camp 2 or Camp 3 (see decision 13)
9. Draw helpers (`filled_rounded_rect`, `stroked_rounded_rect`, `centered_text`, `focus_ring`)
10. Input helpers (`create_encoder_group`, `auto_focus_screen`)

Explicitly **deferred to V2:** theme JSON pipeline, Python theme tooling, canvas-based specialty widgets (waveform, spectrum, meter, dial), reusable helpers beyond widgets, animation presets beyond `lv_anim`.

**Rationale:** Ship what the pilot products need and nothing more. Everything deferred is addable later without breaking the contract.

Making `blusys_framework/core` mandatory is intentional. The platform's value is not only in UI reuse; it is also in forcing a consistent product lifecycle, controller model, feedback model, and application structure across both consumer and industrial/headless devices. Headless products therefore do not get a separate architectural rule set in V1.

---

## 6. Phase 2/3 ordering

**Decision:** Narrow Phase 2 to **identity-only** changes: README and `docs/architecture.md` update their tone from "library" to "platform," and nothing else. All packaging prose (layering model, component diagram, migration notes) moves to Phase 3, which runs after packaging-shape decisions land.

**Rationale:** Writing packaging prose before packaging is decided means rewriting it. Phase 2 stays small and fast; Phase 3 owns the real doc work.

---

## 7. Multi-repo consumption model

**Decision:** Product repos consume the platform as **ESP-IDF managed components with git sources**. All three components pin the **same platform version tag** per release — mixing tiers across versions is not supported.

```yaml
dependencies:
  blusys:
    git: "git@github.com:oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys"
  blusys_services:
    git: "git@github.com:oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys_services"
  blusys_framework:
    git: "git@github.com:oguzkaganozt/blusys.git"
    version: "v5.0.0"
    path: "components/blusys_framework"
```

Platform releases use one semver tag at the repo level (e.g., `v5.0.0`). A release tag covers all three components; they are never versioned independently. The scaffold generates all three dependencies pinned to the same tag. Product teams upgrade by bumping all three version fields simultaneously.

**Rationale:** ESP-IDF's component manager supports git sources natively, so no registry is needed. But all three components live in the same git repo, and a release tag marks one commit. Independent per-tier versioning would let products silently mix components from different commits — components whose internal contracts were never tested together. This is a monorepo platform shipped as one versioned unit, not three independently-evolving libraries. The single-version-per-release rule makes compatibility guaranteed by construction.

---

## 8. Theme pipeline

**Decision:** No JSON, no Python, no build-time generation, no runtime parser. The theme is a single C++ struct populated by the product at boot:

```cpp
namespace blusys::ui {
    struct theme_tokens {
        lv_color_t color_primary, color_surface, color_on_primary,
                   color_accent, color_disabled;
        int spacing_sm, spacing_md, spacing_lg;
        int radius_card, radius_button;
        const lv_font_t *font_body, *font_title, *font_mono;
    };
    void set_theme(const theme_tokens& t);
    const theme_tokens& theme();
}
```

Products populate this struct once at boot (typically in `app/ui/theme_init.cpp`). Every widget reads `theme()` when it draws. The `theme_tokens` struct is **append-only** — new fields get default values so existing products keep building.

If a future need for JSON emerges (e.g., design tool integration), a helper can parse JSON into `theme_tokens`. The widget contract doesn't change because it only knows the struct.

**Rationale:** Zero tooling, zero Python dependency, zero build-time generation complexity. `constexpr`/struct literals are as fast as `#define` with type safety and namespace discipline. The "theme pipeline" collapses to one struct and one setter.

---

## 9. Validation timing

**Decision:** Phase 9 remains the formal validation gate. Earlier phases still get informal spot-checks only (build passes, pilot example runs, no obvious regression). We do **not** add hard per-phase budgets before Phase 9, but the Phase 9 gate must capture a small concrete evidence set:

- reference interactive product runs through PC + SDL2, QEMU, and real hardware
- reference headless product runs through QEMU and real hardware
- input → feedback latency measured on real hardware for the interactive product
- RAM/flash deltas recorded against the pre-migration baseline

**Rationale:** This keeps validation light enough for a small internal team while still producing concrete evidence that the platform is usable on both interactive and headless products.

---

## 10. Success criteria

**Decision:** Keep the success criteria minimal, but not purely qualitative. We still avoid heavy process and hard numeric targets, but a transition is "done" only when the following outcomes and artifacts exist:
- The repo identity is "platform," not "library," in docs and code
- Product teams can start a new product from the scaffold and reach first-boot on hardware
- At least one reference interactive product runs end-to-end on the platform
- At least one reference headless product boots and exercises its primary flow on the platform
- HAL, drivers, and services stay C; framework is C++ (V1). Services migration to C++ deferred to V2.
- The widget kit produces visually-consistent screens across products
- Validation notes capture the interactive-product latency measurement and RAM/flash deltas for the reference products

**Rationale:** The platform is internal, so we keep the bar small and practical. But "done" still needs a little objective evidence so product teams can trust the result.

---

## 11. Documentation language

**Decision:** Normative platform docs (architecture, policy, roadmap, API reference, guides) are in **English**. Working notes, design discussions, retros, and internal memos may stay Turkish. Existing Turkish normative docs get translated to English opportunistically when touched, not in a big-bang translation phase.

**Rationale:** English matches code and API comments, plays well with AI tooling and future contributors, and is the de-facto language of the embedded ecosystem. Turkish stays available for working notes where code-switching is natural.

---

## 12. UI widget architecture

**Decision:** Use LVGL as the rendering/layout/event engine. Build a custom widget kit on top. The kit has two component categories:

**Layout primitives** — composition factories over `lv_obj_create` with styles from tokens. No new widget classes. Examples: `bu_screen`, `bu_row`, `bu_col`, `bu_stack`, `bu_spacer`, `bu_divider`, `bu_label`, `bu_text_value`.

**Interactive widgets** — one `.cpp` per widget, may be implemented as either a stock-LVGL-backed wrapper (Camp 2) or a custom `lv_obj_class_t` subclass (Camp 3). Examples: `bu_button`, `bu_slider`, `bu_toggle`, `bu_modal`, `bu_overlay`, `bu_knob`. The choice is per-widget and invisible to product code.

**Component contract** (every widget obeys):

1. Theme tokens are the only source of visual values. No hex literals, no magic pixel numbers, no inline font pointers in widget draw code.
2. Config struct is the interface. Products pass a `<name>_config` struct at creation. Callbacks use the **semantic callback types** defined in `blusys::ui` (`press_cb_t`, `change_cb_t`, `focus_cb_t`, etc.) — plain function pointers taking `void* user_data`. Products never see `lv_event_cb_t` or `lv_event_t*`. The widget's implementation maps LVGL events to the semantic callback internally (via embedded storage for Camp 3, or a fixed-capacity slot pool for Camp 2).
3. Setters own state transitions. Products call `<name>_set_*` functions; they never call `lv_obj_add_state()` or manipulate styles directly.
4. Standard state set: every interactive widget handles `normal` / `focused` / `pressed` / `disabled` at minimum.
5. One component = one folder (`blusys_framework/ui/widgets/<name>/`) with one `.hpp` and one `.cpp`.
6. The header is the spec. Looking at `<name>.hpp` tells you everything a product can do with that widget.

V1 reduces LVGL exposure in normal product code, but does not attempt to hide LVGL completely. Opaque handles and engine-native value types such as `lv_obj_t*`, `lv_color_t`, `lv_font_t*`, and `lv_group_t*` may still appear in framework headers where they keep the API thin and honest. What products should not see in normal widget APIs are raw LVGL event contracts like `lv_event_cb_t` and `lv_event_t*`.

**Rationale:** LVGL solves rendering, layout, events, and input. Don't rewrite that. The 6-rule contract is what keeps the design system consistent over time — not framework machinery, just conventions held by code review.

---

## 13. Camp 2 / Camp 3 pragmatic split

**Decision:** Each interactive widget's implementation is chosen independently:

- **Camp 2 (stock-LVGL-backed)** — default for widgets where `lv_button` / `lv_slider` / `lv_switch` / etc. can be styled from theme tokens to reach the target look. Implementation wraps the stock widget, applies theme styles, wires semantic callbacks. ~80–120 lines per widget.
- **Camp 3 (custom `lv_obj_class_t` subclass)** — used only when stock LVGL genuinely can't produce the required visual (non-rectangular shapes, custom gradients, per-instance custom rendering, specialty renderings like knobs and meters). ~150–300 lines per widget.

The product-facing header is identical in both cases, so an implementation can **migrate between Camp 2 and Camp 3 without touching product code**. Migration is per-widget, not per-kit.

V1 target mix (subject to pilot feedback):
- Layout primitives: 8 (composition factories, neither Camp 2 nor Camp 3)
- Camp 2 stock-backed widgets: ~5 (`bu_button`, `bu_slider`, `bu_toggle`, `bu_modal`, `bu_overlay`)
- Camp 3 custom-class widgets: 0–2 (`bu_knob` only if a pilot product needs it; otherwise deferred)

**Rationale:** Pure Camp 3 (everything is a custom class) is overkill for widgets that are just rounded rects with labels. Pure Camp 2 (everything wraps stock LVGL) caps visual distinctiveness. The pragmatic split pays the custom-class cost only where it buys real visual ownership.

---

## 14. Logging convention

**Decision:** Framework and any new platform code use a thin `blusys/log.h` wrapper over ESP-IDF's `esp_log.h`. HAL and services continue to use `esp_log.h` directly (no churn). New framework code uses `BLUSYS_LOGE/I/W/D(tag, fmt, ...)` macros defined in `components/blusys/include/blusys/log.h`:

```c
// components/blusys/include/blusys/log.h
#pragma once
#include "esp_log.h"

#define BLUSYS_LOGE(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define BLUSYS_LOGW(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define BLUSYS_LOGI(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define BLUSYS_LOGD(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)
```

The wrapper is Phase 4 infrastructure. It exists so the framework's C++ code can log without the widget-kit documentation or any future platform code having to reach for `esp_log.h` directly — preserving a single platform-facing name for logging.

**Rationale:** Framework needs logging, and the Camp 2 reference implementation in `UI-system.md` already uses `BLUSYS_LOGE`. A single-file wrapper is trivially cheap and gives one place to change the backend later if needed (e.g., to route through a feedback channel during validation, or to gate verbosity via Kconfig). HAL and services don't migrate: their current `static const char *TAG = "..."` + `ESP_LOGE(TAG, ...)` pattern is fine, and churn-for-churn's-sake is forbidden by the plan's minimalism policy.

---

## 15. UI gating mechanism

**Decision:** `BLUSYS_STARTER_TYPE=interactive` toggles `blusys_framework/ui` compilation via a `BLUSYS_BUILD_UI` CACHE variable set by the product's top-level CMakeLists.txt **before** `project()`.

The full scaffold-generated top-level CMakeLists.txt:

```cmake
# Product top-level CMakeLists.txt (scaffold-generated)
cmake_minimum_required(VERSION 3.16)

# Read product configuration
include(${CMAKE_CURRENT_LIST_DIR}/app/product_config.cmake)

# Derive build flags from starter type
if(BLUSYS_STARTER_TYPE STREQUAL "interactive")
    set(BLUSYS_BUILD_UI ON CACHE BOOL "Build blusys_framework/ui")
else()
    set(BLUSYS_BUILD_UI OFF CACHE BOOL "Build blusys_framework/ui")
endif()

# Tell ESP-IDF to scan the project root for extra components. This is how
# the local `app/` component (which owns product sources under app/ui, 
# app/controllers, app/integrations) gets discovered. The platform 
# components (blusys, blusys_services, blusys_framework) are NOT pulled 
# in via this mechanism — they come from main/idf_component.yml through
# ESP-IDF's managed component manager (see decision 7).
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${BLUSYS_PRODUCT_NAME})
```

`components/blusys_framework/CMakeLists.txt` consumes the cache variable:

```cmake
set(srcs
    src/core/router.cpp
    src/core/intent.cpp
    src/core/feedback.cpp
    src/core/controller.cpp
    src/core/framework.cpp
)

if(BLUSYS_BUILD_UI)
    list(APPEND srcs
        src/ui/theme.cpp
        src/ui/primitives/row.cpp
        # ... rest of widget kit
    )
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS include
    REQUIRES blusys blusys_services
)
```

**Rationale:** CACHE variables are visible in every CMake scope including component CMakeLists during ESP-IDF's component scan — no `idf_build_set_property` gymnastics, no Kconfig surface. `app/product_config.cmake` is the primary starter-type input at scaffold time, and the top-level CMake stays derived from it. V1 deliberately stops there: scaffolded files are normal source files after creation, not part of a live regenerate/sync system. That keeps the model straightforward for a small internal team. Flow is still strictly one-way (product → framework); framework never reaches up into `app/`. Headless products that try to include UI headers get link errors for undefined symbols, which is the desired failure mode.

The `EXTRA_COMPONENT_DIRS` line points at the project root so ESP-IDF discovers the local `app/` component. It deliberately does **not** point at `$ENV{BLUSYS_PATH}/components`: product repos consume the platform through managed components pinned in `main/idf_component.yml` (decision 7). Mixing the two models (managed components for platform + `EXTRA_COMPONENT_DIRS` to the monorepo for platform) would silently prefer local checkouts over the pinned tag — the opposite of what a product repo should do.

Rejected alternatives:
- **Kconfig symbol (`CONFIG_BLUSYS_FRAMEWORK_UI_ENABLED`):** adds a menuconfig surface product teams would otherwise never touch, and forces configuration in both `product_config.cmake` and `sdkconfig.defaults`.
- **Framework reads `app/product_config.cmake` directly via `include()`:** upward dependency — framework would need to know the product layout exists. Architecturally dirty.
- **`EXTRA_COMPONENT_DIRS` pointing at `$ENV{BLUSYS_PATH}/components`:** contradicts the managed-components consumption model (decision 7). This is the pattern the monorepo's internal `examples/` directory uses, but it is not appropriate for scaffolded product repos. Platform maintainers who need to test a scaffolded product against a local platform checkout should use `idf_component.yml`'s `override_path` feature instead.

---

## 16. HAL/drivers separation enforcement (CI lint)

**Decision:** Because drivers live inside `components/blusys/` alongside HAL (decision 4), layering is enforced by a minimal CI lint check rather than the component boundary. The check runs in CI and locally via `blusys lint` (added to the CLI in Phase 3) and fails the build if:

1. Any file under `components/blusys/src/common/` includes a header matching `blusys/drivers/**`.
2. Any file under `components/blusys/src/drivers/` includes a header matching `blusys/internal/**` **except** the allowlist: `blusys/internal/blusys_lock.h`, `blusys/internal/blusys_esp_err.h`, `blusys/internal/blusys_timeout.h`.

The allowlist is closed for the driver tier. Adding a header to it requires an explicit decision recorded in this file, because internal headers are the de-facto ABI between HAL and the tiers above it.

`components/blusys_services/` is intentionally **not** part of this lint in V1. Existing services legitimately share helpers such as BT/NVS setup internals, and forcing a stricter automated boundary now would add refactoring churn without helping a 2-3 person internal team move faster.

**Rationale:** Directory discipline alone catches most violations in review, but humans miss things. A 30-line shell or Python grep-based lint is cheap enforcement and turns the most important convention into a guarantee. The check is fast, has no external dependencies, and runs identically in CI and on developer machines. It is deliberately much smaller than full static analysis — it only encodes the HAL/drivers layering rule, nothing else.

---

## Changelog

- **v1.0** — Initial decision log. All 13 decisions locked following the platform-transition review discussion.
- **v1.1** — Pre-implementation refinement pass. Decision 1 narrowed: C++ scoped to framework only in V1; services stays C (migration deferred to V2). Decision 2 wording updated (framework-only). Decision 4 collapsed: three tiers (drivers inside `components/blusys/` with directory + CI lint enforcement) instead of four. Decision 7 git URL corrected to `oguzkaganozt/blusys`. Added decisions 14 (logging wrapper), 15 (UI gating mechanism), 16 (CI lint for HAL/drivers separation).
- **v1.2** — Pragmatic V1 refinement. `usb_hid` stays in `blusys_services`, scaffold behavior clarified as one-time generation rather than live sync, LVGL exposure policy softened (reduce, not eliminate), validation now requires a small concrete evidence set, and the CI lint scope is narrowed to HAL/drivers only.

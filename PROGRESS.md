# Progress

## Current Summary

- current track: `Platform transition`
- current phase: `Phase 9 in progress — stages 1 (host) + 2 (QEMU) landed, migration guide + validation report written, stage 3 (hardware) blocked on physical boards`
- target release: `v6.0.0`
- overall status: `in_progress`

Blusys is mid-transition from a HAL/services library repo into an internal embedded product platform. The planning source of truth lives under [`platform-transition/`](platform-transition/).

- roadmap: [`platform-transition/ROADMAP.md`](platform-transition/ROADMAP.md)
- decisions: [`platform-transition/DECISIONS.md`](platform-transition/DECISIONS.md)
- architecture: [`platform-transition/application-layer-architecture.md`](platform-transition/application-layer-architecture.md)
- C++ policy: [`platform-transition/CPP-transition.md`](platform-transition/CPP-transition.md)
- UI system: [`platform-transition/UI-system.md`](platform-transition/UI-system.md)

## Transition Status

| # | Phase | Status |
|---|---|---|
| 1 | Lock platform positioning | completed |
| 2 | Identity alignment | completed |
| 3 | Packaging shape | completed |
| 4 | C++ infrastructure (framework only) + `blusys/log.h` | completed |
| 4.5 | PC + SDL2 host harness | completed |
| 5 | Flagship widget and V1 widget kit | substantially_complete (bu_knob deferred to V2 per decision) |
| 6 | Framework core V1 | completed |
| 7 | Product scaffold and sample apps | completed |
| 8 | Example ecosystem migration | completed |
| 9 | Validation and migration notes | in_progress (stages 1+2 landed, stage 3 hardware gate pending) |

## Locked Direction

- three tiers in V1: `blusys` (HAL + drivers, C) -> `blusys_services` (C) -> `blusys_framework` (C++)
- framework is the only C++ tier in V1; services-to-C++ migration is deferred to V2
- drivers move under `components/blusys/src/drivers/` and are separated from HAL by directory rules + CI lint
- UI is optional and gated by `BLUSYS_BUILD_UI`, derived from `BLUSYS_STARTER_TYPE`
- product repos consume the platform through managed components pinned to one repo tag per release

## Active Review Findings

No open issues. The planning docs have gone through three review passes and all flagged items are resolved:

- Pass 1: C++20 baseline, component-granular build, single-version pinning, semantic callback types.
- Pass 2: managed-components vs `EXTRA_COMPONENT_DIRS` consumption model unified; `app/` scaffold compilation specified via its own ESP-IDF component; Camp 3 widget template fixed to call `lv_obj_class_init_obj`; bootstrap unified to `main/app_main.cpp` for both profiles; widget naming normalized.
- Pass 3: sample screen include path corrected to match `INCLUDE_DIRS "."` convention; `ui/widgets` added to the scaffold template; product-facing authoring surface reframed to distinguish `product_config.cmake` from scaffold-generated files (including the conditional `lvgl/lvgl` managed dep).

## Landed Implementation

- repo/docs identity updated from HAL-only wording to platform transition wording
- driver modules moved into `components/blusys/src/drivers/` with public headers under `components/blusys/include/blusys/drivers/`
- `usb_hid` kept in `blusys_services` and build-fixed for current `usb_host_hid` APIs
- `blusys lint` added to enforce the HAL/drivers boundary
- example includes and `REQUIRES` entries updated for the driver move
- `components/blusys_framework/` added as a real component with C++20 compile policy
- foundational framework files added: `framework.hpp`, `core/containers.hpp`, `framework.cpp`, `README.md`
- `blusys/log.h` added for framework-side logging
- framework core contracts added: `core/router.hpp`, `core/intent.hpp`, `core/feedback.hpp`, `core/controller.hpp`
- framework runtime added: `core/runtime.hpp` with queued events, route delivery, feedback bus integration, and tick cadence ownership
- framework UI foundation added: `ui/theme.hpp`, `ui/widgets.hpp`, and first layout primitives (`screen`, `row`, `col`, `label`, `divider`)
- framework UI semantic callbacks header added: `ui/callbacks.hpp` with `press_cb_t`, `release_cb_t`, `long_press_cb_t`, `toggle_cb_t`, `change_cb_t`
- first interactive widget added: `bu_button` (Camp 2 stock-backed wrapper around `lv_button`) in `ui/widgets/button/`, with the fixed-capacity slot pool, fail-loud assert on exhaustion, and `set_label` / `set_variant` / `set_disabled` setters
- second interactive widget added: `bu_toggle` (Camp 2 stock-backed wrapper around `lv_switch`) in `ui/widgets/toggle/`, mirroring the `bu_button` slot-pool pattern with `set_state` / `get_state` / `set_disabled` setters and a `toggle_cb_t` value-changed callback that fires only on user-driven flips
- third interactive widget added: `bu_slider` (Camp 2 stock-backed wrapper around `lv_slider`) in `ui/widgets/slider/`, validating the slot-pool pattern against a continuous-value stock widget — `set_value` / `get_value` / `set_range` / `set_disabled` setters and a `change_cb_t(int32_t, void *)` callback driven from `LV_EVENT_VALUE_CHANGED`
- fourth interactive widget added: `bu_modal` (themed composition — full-screen dimmed backdrop + centered card panel with optional title/body) in `ui/widgets/modal/`, applying the slot-pool pattern to a non-stock-wrapped composition. `modal_show` / `modal_hide` / `modal_is_visible` setters; backdrop click → `on_dismiss` callback only when the click target is the backdrop itself (target vs current_target check), so panel-area clicks are absorbed. Modals are created hidden by convention.
- fifth interactive widget added: `bu_overlay` (themed transient toast surface) in `ui/widgets/overlay/`, validating the slot-pool pattern against an `lv_timer`-driven auto-dismiss lifecycle. Bottom-anchored compact bubble using `color_primary` background. Slot is always allocated (it must hold the active timer pointer). One-shot LVGL timer (created in `overlay_show`, cancelled on `overlay_hide` / `LV_EVENT_DELETE`) hides the overlay and fires `on_hidden` after `duration_ms`. `duration_ms = 0` disables auto-dismiss for manual control.
- framework UI input helpers added: `ui/input/encoder.hpp` + `encoder.cpp` with `create_encoder_group()` (thin lv_group_create wrapper with error logging) and `auto_focus_screen(screen, group)` (DFS pre-order walk that prunes hidden subtrees and adds every `LV_OBJ_FLAG_CLICKABLE` descendant to the focus group, then focuses the first hit). Pure LVGL group/focus wiring — products own their own `lv_indev_t` for the encoder hardware and bind it to the returned group via `lv_indev_set_group`.
- framework widget author guide added: `components/blusys_framework/widget-author-guide.md` documenting the six-rule contract, slot-pool discipline, setter rules, callback idioms, and a per-widget review checklist
- framework examples added: `examples/framework_core_basic/`, `examples/framework_ui_basic/`, and `examples/framework_app_basic/`. The app example **closes the Phase 6 done-when criterion**: it builds a real screen with the widget kit, defines an `app_controller` that mutates the slider on `intent::increment`/`decrement` and submits `route::show_overlay` on `intent::confirm`, wires a `ui_route_sink` that translates `show_overlay` route commands into `overlay_show` calls on the actual widget, registers a logging `feedback_sink`, and exercises the full chain (`button on_press` → `runtime.post_intent` → `runtime.step` → `controller.handle` → `slider_set_value` / `submit_route` → `ui_route_sink` → `overlay_show`, with feedback emitted at every step). Three buttons (`Down`/`Up`/`OK`) act as the simulated encoder source.
- **Phase 4.5 PC + SDL2 host harness** added under `scripts/host/`: a CMake project that fetches LVGL v9.2.2 from upstream via `FetchContent` (pinned to the same tag as the ESP-IDF managed component), wires it against the system SDL2 via `pkg-config`, filters out the ARM-only `.S` blend back-ends so the build works on x86_64, and registers a `hello_lvgl` smoke test that opens a 480×320 SDL2 window with a centered rounded card. Host-side `lv_conf.h` enables `LV_USE_SDL=1`, 32-bit color, `LV_USE_OS=LV_OS_NONE`, `LV_USE_LOG` with `printf` output, and a 1 MB memory budget. New `blusys host-build` CLI subcommand wraps `cmake -S scripts/host -B scripts/host/build-host && cmake --build` and pre-checks for `cmake`, `pkg-config`, and `sdl2.pc` with friendly error messages. `scripts/host/README.md` documents install steps for apt/dnf/pacman/brew + a troubleshooting section.
- **Phase 7 product scaffold:** `blusys create` extended with `--starter <headless|interactive>` (default: `interactive`). Generates the four-CMakeLists product layout (top-level + `main/` + `app/` + `app/product_config.cmake`), `main/app_main.cpp` (always `.cpp`, no `blusys_all.h`), `main/idf_component.yml` pinning all three platform components to `v6.0.0` from `https://github.com/oguzkaganozt/blusys.git` (plus `lvgl/lvgl ~9.2` for interactive), a sample `app/controllers/home_controller.{hpp,cpp}` that handles `increment`/`decrement`/`confirm` intents and emits feedback, and (for interactive) `app/ui/theme_init.{hpp,cpp}` + `app/ui/screens/main_screen.{hpp,cpp}` that build a `[-]/[+]` counter screen on top of the widget kit. The top-level CMakeLists derives `BLUSYS_BUILD_UI` from `BLUSYS_STARTER_TYPE` so headless products skip the framework UI tier entirely. **Spec correction:** decision 15's example points `EXTRA_COMPONENT_DIRS` at `${CMAKE_CURRENT_LIST_DIR}` (the project root), but ESP-IDF's `__project_component_dir` helper at `tools/cmake/project.cmake:432` treats a path with a `CMakeLists.txt` as a component itself — so the project root would recursively include the top-level CMakeLists, blowing up `component_get_requirements.cmake`. The scaffold points at `${CMAKE_CURRENT_LIST_DIR}/app` instead, which the same helper correctly registers as a single component. Decision 15 amended in-place.
- **Phase 8 example ecosystem migration + `blusys_all.h` removal:** swept the 13 known references (1 example + 12 guides) to per-tier umbrella headers (`blusys/blusys.h`, `blusys/blusys_services.h`, plus a couple of driver-specific tightenings — `blusys/drivers/display/lcd.h` for `lcd-basic.md` and `blusys/drivers/input/button.h` for `button-basic.md`). Deleted `components/blusys_services/include/blusys/blusys_all.h`. Updated the two prose mentions (`docs/architecture.md:182` dropped, `docs/guides/framework.md:197` rewritten to mark Phase 8 closed). Added the missing fourth framework example, `examples/framework_encoder_basic/`, which wires a real `lv_indev_t` of type `LV_INDEV_TYPE_ENCODER` into `blusys::ui::create_encoder_group` + `auto_focus_screen` and bridges the `blusys_encoder` driver (PCNT-backed on ESP32/S3, GPIO-ISR on C3) to LVGL via a callback-to-poll state buffer; example uses a stub display so it builds and runs on all three targets without LCD hardware. Tightened `framework_ui_basic` comments to make explicit it covers both layout-only and interactive-widgets scenarios so the four examples remain non-overlapping.
- **Phase 9 stages 1+2 + deliverables (hardware gate pending):** bridged `blusys_framework` into the host harness (`scripts/host/CMakeLists.txt` now compiles `libblusys_framework_host.a` from all 18 framework sources + `blusys/src/common/error.c`, gated C++-only flags with `COMPILE_LANGUAGE:CXX` generator expressions, and added a minimal `include_host/esp_log.h` shim that maps ESP_LOG* to printf). New host executable `widget_kit_demo` links 46 `blusys::ui::*` symbols + 74 `blusys::framework::*` symbols and exercises the full runtime → controller → route sink chain through `bu_button`, `bu_slider`, `bu_overlay`, and the layout primitives. Stage 2 QEMU validation: `phase9_headless` (scaffolded via `blusys create --starter headless`) and `examples/framework_core_basic` both boot clean on esp32 / esp32c3 / esp32s3 under ESP-IDF QEMU 9.2.2 with the expected spine events (no panics, no LoadProhibited). RAM/flash deltas vs pre-migration baseline (esp32s3): framework core ≈ +2 KB total image / +0.5 KB RAM on top of bare HAL; UI tier (LVGL + widget kit) ≈ +136 KB total image / +66 KB `.bss` — LVGL dominates, Blusys widget kit itself is only 3,355 bytes total. Interactive product cannot run in QEMU (no LCD emulation) — its device output path is a stage-3 deliverable. Full report: [`platform-transition/phase9-validation-report.md`](platform-transition/phase9-validation-report.md). V5→V6 migration guide written at [`docs/migration-guide.md`](docs/migration-guide.md) and added to the mkdocs Project tab; `mkdocs build --strict` passes.
- docs/nav updated to reflect HAL + Drivers + Services + Framework structure

## Verification Snapshot

- `./blusys lint` passes
- `./blusys build examples/button_basic esp32s3` passes
- `./blusys build examples/ui_basic esp32s3` passes
- `./blusys build examples/usb_hid_basic esp32s3` passes
- `./blusys build examples/framework_core_basic esp32s3` passes
- `./blusys build examples/framework_ui_basic esp32s3` passes
- `./blusys build examples/framework_app_basic esp32s3 / esp32 / esp32c3` passes (Phase 6 end-to-end validation)
- `./blusys build examples/framework_encoder_basic esp32s3 / esp32 / esp32c3` passes (Phase 8: real lv_indev_t encoder traversal). Binary sizes: esp32s3 0x6cdb0 (57% free), esp32 0x671e0 (60% free), esp32c3 0x6b620 (58% free).
- `./blusys host-build` passes on Linux (Ubuntu 24.04, SDL2 2.30.0): CMake configures, `FetchContent` pulls lvgl v9.2.2, lvgl builds with `.S` files filtered, `hello_lvgl` (955 KB ELF) links against `libSDL2-2.0.so.0` and `libm`. Window-rendering still requires a manual interactive run since CI / headless shells can't open an SDL display.
- `./blusys create --starter interactive my_panel` + `./blusys build my_panel esp32s3` passes against the local platform checkout via `override_path` (interactive ELF: `my_panel.bin` 0x50600 bytes, 69% free; 17 `blusys::ui::*` symbols linked).
- `./blusys create --starter headless my_gateway` + `./blusys build my_gateway esp32s3` passes against the local platform checkout via `override_path` (headless ELF: `my_gateway.bin` 0x2f210 bytes, 82% free; **zero** `blusys::ui::*` symbols linked — confirms the `BLUSYS_BUILD_UI` gate excludes the entire UI tier from headless builds).
- **Phase 9 stage 1 (host):** `./blusys host-build` passes on Ubuntu 24.04. Two executables produced: `hello_lvgl` (977 KB) and `widget_kit_demo` (1,011 KB, +34 KB over hello). `libblusys_framework_host.a` is 94 KB. `widget_kit_demo` runs headlessly under `SDL_VIDEODRIVER=dummy` without crashing and logs the expected initial controller feedback + slider value through the framework spine.
- **Phase 9 stage 2 (QEMU, esp-qemu 9.2.2):** `phase9_headless` boots clean on esp32 / esp32c3 / esp32s3 with the expected `I (app_main) headless product running` log, no panics. `examples/framework_core_basic` runs the full spine on all three targets with exactly 5 expected events (`visual/focus`, `audio/click`, `route:set_root id=1`, `visual/confirm`, `route:show_overlay id=7`), no panics.
- `mkdocs build --strict` passes (includes the new `docs/migration-guide.md` under the Project tab).

## Immediate Next Actions

1. **Phase 9 stage 3 — real hardware (hardware gate, blocks `v6.0.0` tag).** Requires physical boards. Run on all three targets:
   - **first-boot success** for `phase9_headless` (UART log confirms `headless product running`);
   - **first-boot success** for `phase9_interactive` on an esp32s3 + LCD + encoder rig (screen renders, encoder rotation moves slider, encoder press shows overlay);
   - **input → feedback latency measurement** on the interactive product (logic analyser or scope), recorded per `docs/guides/hardware-validation-report-template.md`.
   File reports under `docs/guides/` following the Phase 5 naming convention. When all three land, flip Phase 9 to `completed` and update `platform-transition/phase9-validation-report.md` §1–§5 with the results.
2. **Tag `v6.0.0` once stage 3 lands** so scaffolded products can pull the platform from the canonical git URL without `override_path`.
3. **(Optional nice-to-have)** Add keyboard-driven encoder simulation to `scripts/host/src/widget_kit_demo.cpp` — map arrow keys to `LV_INDEV_TYPE_ENCODER` events so encoder focus traversal can be validated visually on host without real hardware. Currently only `framework_encoder_basic` does this on-device.
4. Keep `platform-transition/` and repo guidance docs aligned as stage-3 reports land.

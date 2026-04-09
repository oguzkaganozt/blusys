# Progress

## Current Summary

- current track: `Platform transition`
- current phase: `Phase 6 closed — V1 spine + widget kit + end-to-end app validated; next: Phase 4.5 SDL2 harness`
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
| 4.5 | PC + SDL2 host harness | pending |
| 5 | Flagship widget and V1 widget kit | substantially_complete (bu_knob deferred to V2 per decision) |
| 6 | Framework core V1 | completed |
| 7 | Product scaffold and sample apps | pending |
| 8 | Example ecosystem migration | pending |
| 9 | Validation and migration notes | pending |

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
- docs/nav updated to reflect HAL + Drivers + Services + Framework structure

## Verification Snapshot

- `./blusys lint` passes
- `./blusys build examples/button_basic esp32s3` passes
- `./blusys build examples/ui_basic esp32s3` passes
- `./blusys build examples/usb_hid_basic esp32s3` passes
- `./blusys build examples/framework_core_basic esp32s3` passes
- `./blusys build examples/framework_ui_basic esp32s3` passes
- `./blusys build examples/framework_app_basic esp32s3 / esp32 / esp32c3` passes (Phase 6 end-to-end validation)
- `mkdocs build --strict` passes

## Immediate Next Actions

1. **Phase 4.5 — PC + SDL2 host harness.** Stand up `scripts/host/` with a CMake project that builds LVGL against SDL2 on Linux, pinned to the same lvgl/lvgl version as the managed component (~9.2). First milestone: a "hello LVGL" host target that opens an SDL2 window. Then a `blusys host-build` CLI subcommand and a `scripts/host/README.md` documenting the toolchain setup. **Blocker check:** the host build needs `libsdl2-dev` (or equivalent) installed on the dev machine — confirm the dev environment before running the host build.
2. **Phase 7 — Product scaffold.** Extend `blusys create` with `--starter <headless|interactive>` and generate the four-CMakeLists scaffold (top-level + main + app + product_config.cmake). Defer until Phase 4.5 lands so the scaffold can be iterated against the host harness rather than flashing hardware on every change.
3. Keep `platform-transition/` and repo guidance docs aligned as framework work lands.

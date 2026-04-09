# Framework

`components/blusys_framework/` is the C++ tier of the platform. It sits above
`blusys_services` and ships the V1 product surface in full: a core spine for
controllers, routing, intents, and feedback; a themed widget kit built on
LVGL; encoder focus helpers; and an end-to-end app example that exercises the
full chain.

The umbrella header is `blusys/framework/framework.hpp` (core only). The UI
layer has its own umbrella at `blusys/framework/ui/widgets.hpp` and is gated
by `BLUSYS_BUILD_UI`.

## Core spine

| Header | Purpose |
|---|---|
| `blusys/framework/core/router.hpp` | Six route commands (`set_root`, `push`, `replace`, `pop`, `show_overlay`, `hide_overlay`) plus a `route_sink` interface. Constexpr factory helpers under `blusys::framework::route::*`. |
| `blusys/framework/core/intent.hpp` | Nine semantic intents (`press`/`long_press`/`release`/`confirm`/`cancel`/`increment`/`decrement`/`focus_next`/`focus_prev`) and the `app_event` envelope. |
| `blusys/framework/core/feedback.hpp` | Three channels (`visual`/`audio`/`haptic`), six patterns, fixed-capacity `feedback_bus` with up to six sinks. |
| `blusys/framework/core/controller.hpp` | `init` / `handle` / `tick` / `deinit` lifecycle base class. `submit_route` and `emit_feedback` helpers route through the bound runtime. |
| `blusys/framework/core/runtime.hpp` | Event ring buffer (16), route ring buffer (8), 10 ms default tick cadence. The runtime is itself a `route_sink` and forwards to a product-supplied output sink. Registers feedback sinks. |
| `blusys/framework/core/containers.hpp` | `static_vector`, `ring_buffer`, `array`, `string` — fixed-capacity, no dynamic allocation. |

## Widget kit (V1)

The widget kit follows a six-rule component contract — see
`components/blusys_framework/widget-author-guide.md`. Each Camp 2 widget wraps
a stock LVGL widget, applies theme tokens, and stores its callback in a
fixed-capacity slot pool keyed by `BLUSYS_UI_<NAME>_POOL_SIZE` (default 32).
Pool exhaustion is a hard fail-loud error.

### Layout primitives

| Header | Stock backing | Notes |
|---|---|---|
| `ui/primitives/screen.hpp` | `lv_obj_create(nullptr)` | Themed full-screen container. |
| `ui/primitives/row.hpp` | `lv_obj_create` + flex row | `gap`, `padding`. |
| `ui/primitives/col.hpp` | `lv_obj_create` + flex column | `gap`, `padding`. |
| `ui/primitives/label.hpp` | `lv_label_create` | Optional font override. |
| `ui/primitives/divider.hpp` | `lv_obj_create` | Themed horizontal rule. |

### Interactive widgets

| Header | Stock backing | Callback | Setters |
|---|---|---|---|
| `ui/widgets/button/button.hpp` | `lv_button` | `press_cb_t` | `set_label`, `set_variant`, `set_disabled` |
| `ui/widgets/toggle/toggle.hpp` | `lv_switch` | `toggle_cb_t` | `set_state`, `get_state`, `set_disabled` |
| `ui/widgets/slider/slider.hpp` | `lv_slider` | `change_cb_t` | `set_value`, `get_value`, `set_range`, `set_disabled` |
| `ui/widgets/modal/modal.hpp` | composition | `press_cb_t` (dismiss) | `show`, `hide`, `is_visible` |
| `ui/widgets/overlay/overlay.hpp` | composition + `lv_timer` | `press_cb_t` (hidden) | `show`, `hide`, `is_visible` |

`bu_knob` is intentionally deferred to V2 unless a pilot product asks for it.

### Theme

`ui/theme.hpp` exposes `theme_tokens` (colors, spacing, radius, fonts), the
`set_theme()` setter, and the `theme()` accessor. Products populate the
struct once at boot. The struct is append-only — never rename or remove a
field.

### Semantic callbacks

`ui/callbacks.hpp` defines the function pointer types widgets accept in
their config structs: `press_cb_t`, `release_cb_t`, `long_press_cb_t`,
`toggle_cb_t`, `change_cb_t`. Products never see `lv_event_cb_t` or
`lv_event_t`. Captureless lambdas decay to function pointers via the `+[]`
idiom; pass state through `user_data`.

### Encoder focus helpers

`ui/input/encoder.hpp`:

- `create_encoder_group()` — wraps `lv_group_create` with error logging.
- `auto_focus_screen(screen, group)` — DFS pre-order walk that prunes hidden
  subtrees and adds every `LV_OBJ_FLAG_CLICKABLE` descendant to the focus
  group, then focuses the first hit.

Pure LVGL group/focus wiring — products own their own `lv_indev_t` for the
encoder hardware and bind it to the returned group via `lv_indev_set_group`.

## Examples

| Example | What it shows |
|---|---|
| `examples/framework_core_basic/` | Core spine in isolation: a logging route sink + feedback sink, a demo controller posting intents, runtime stepping the event queue. No UI. |
| `examples/framework_ui_basic/` | UI layer in isolation: theme setup, screen + layout primitives, every widget in the kit (button, toggle, slider, modal, overlay), and encoder helpers. No spine — widget callbacks log directly. |
| `examples/framework_app_basic/` | **End-to-end Phase 6 validation.** Builds a screen with the widget kit, defines an `app_controller` that mutates the slider on `intent::increment`/`decrement` and submits `route::show_overlay` on `intent::confirm`, wires a `ui_route_sink` that translates `show_overlay` into `overlay_show`, registers a logging `feedback_sink`, and exercises the full chain (`button on_press` → `runtime.post_intent` → `runtime.step` → `controller.handle` → `slider_set_value` / `submit_route` → `ui_route_sink` → `overlay_show`, with feedback emitted at every step). |

## Host iteration

The framework UI can be iterated on host without flashing hardware via
`scripts/host/`, which builds LVGL against SDL2 on Linux. See
[`scripts/host/README.md`](https://github.com/oguzkaganozt/blusys/blob/main/scripts/host/README.md)
for install steps. Run `blusys host-build` to configure and build, then
`./scripts/host/build-host/hello_lvgl` to launch the smoke test.

## Authoring rules

Every widget added to `blusys_framework/ui/widgets/` obeys the six-rule
contract codified in
[`components/blusys_framework/widget-author-guide.md`](https://github.com/oguzkaganozt/blusys/blob/main/components/blusys_framework/widget-author-guide.md):

1. Theme tokens are the only source of visual values.
2. The config struct is the interface — no LVGL event types in the public
   surface.
3. Setters own state transitions.
4. Standard four states: `normal`, `focused`, `pressed`, `disabled`.
5. One component = one folder.
6. The header is the spec — no hidden behaviour in the `.cpp`.

For the layering rules and dependency direction, see
[Architecture](../architecture.md).

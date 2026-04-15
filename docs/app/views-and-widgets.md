# Views & Widgets

The Blusys view layer provides stock widgets and a custom widget contract that keeps common apps off raw LVGL.

## Page Helpers

Use `page_create()` and `page_load()` to build and activate a screen:

```cpp
void on_init(blusys::app_ctx &ctx, blusys::app_services &svc, State &state)
{
    (void)svc;
    auto p = blusys::page_create();  // creates screen + content col + focus group

    // Populate p.content with widgets ...

    blusys::page_load(p);            // activates screen and wires encoder focus
}
```

`page_create()` accepts an optional `page_config` to set padding, gap, and scrollability.

Inside the interaction shell, `page_create_in(shell.content_area, …)` produces a bounded scroll column (`flex_grow`, optional scroll) so chrome (tabs, header) stays on screen. For LVGL flex conventions, primitives (`blusys::row`, `blusys::col`), and sizing helpers used by stock widgets, see [Architecture — UI layout](../internals/architecture.md#ui-layout-lvgl-flex).

## Navigation

In `on_init`, register screens with `ctx.services().screen_router()->register_screen(route_id, create_fn, destroy_fn)` (optionally with lifecycle hooks). Move between screens with `ctx.services().navigate_to`, `ctx.services().navigate_push`, `ctx.services().navigate_replace`, and `ctx.services().navigate_back()`. Overlays use `ctx.services().show_overlay` / `hide_overlay`. The framework owns the route queue, stack, transitions, and focus reattachment after each change — product code does not touch `route_sink` or raw `route_command` values except through **`app_services`** (`ctx.services()`).

## Stock Widgets

All widget helpers take the parent container as the first argument.

### Umbrella includes

- `blusys/framework/ui/primitives.hpp` — layout primitives only (`screen`, `row`, `col`, `label`, `divider`, `icon_label`, `status_badge`, `key_value`).
- `blusys/framework/ui/widgets.hpp` — primitives plus the full stock widget set (buttons through `vu_strip`). Prefer this in product code rather than deep one-off includes.

`blusys/framework/ui/binding/action_widgets.hpp` exposes ergonomic factories (e.g. `blusys::button`, `blusys::progress`) that wrap the underlying `blusys::*_create` APIs.

### Inventory (by category)

| Category | Stock kit |
|----------|-----------|
| Layout primitives | `screen`, `row`, `col`, `label`, `divider`, `icon_label`, `status_badge`, `key_value` |
| Interactive | `button`, `toggle`, `slider`, `modal`, `overlay`, `list`, `tabs`, `dropdown`, `input_field`, `knob` |
| Display | `progress`, `card`, `gauge`, `chart`, `data_table`, `level_bar`, `vu_strip` |

Authoring rules for contributors live in `docs/internals/architecture.md` (widget kit section).

### Labels and headings

```cpp
blusys::title(p.content, "Settings");      // large heading label
blusys::label(p.content, "WiFi: off");     // body label — returns lv_obj_t*

// Update text from the reducer:
lv_obj_t *lbl = blusys::label(p.content, "0");
// ... later in update():
blusys::set_text(lbl, "42");
```

### Buttons

Buttons dispatch a product action when pressed:

```cpp
blusys::button(p.content, "Increment",
             Action{.tag = Tag::increment}, &ctx);

// Secondary style variant:
blusys::button(p.content, "Reset",
             Action{.tag = Tag::reset}, &ctx,
             blusys::ui::button_variant::secondary);
```

### Slider

```cpp
blusys::slider(p.content, 0, 100, state.brightness,
             [](int32_t v, void *user) {
                 auto *ctx = static_cast<blusys::app_ctx *>(user);
                 ctx->dispatch(Action{.tag = Tag::set_brightness, .value = v});
             },
             &ctx);
```

### Toggle

```cpp
blusys::toggle(p.content, state.enabled,
             [](bool v, void *user) {
                 auto *ctx = static_cast<blusys::app_ctx *>(user);
                 ctx->dispatch(Action{.tag = Tag::set_enabled, .value = v});
             },
             &ctx);
```

### Divider and Row

```cpp
blusys::divider(p.content);              // horizontal separator
auto *row = blusys::row(p.content);      // horizontal flex container
blusys::button(row, "-", ...);
blusys::button(row, "+", ...);
```

### Level bar and VU strip (audio-style displays)

Display-only helpers for compact level feedback. Update values from the reducer with `blusys::ui::level_bar_set_value` / `blusys::ui::vu_strip_set_value`, or keep handles and use `blusys::set_*` bindings where applicable.

```cpp
lv_obj_t *lvl = blusys::level_bar(p.content, 0, 100, state.level, "Output");
lv_obj_t *vu  = blusys::vu_strip(p.content, 12, 0, blusys::ui::vu_orientation::vertical);
// ... in update(), after state changes:
blusys::ui::level_bar_set_value(lvl, state.level);
blusys::ui::vu_strip_set_value(vu, static_cast<std::uint8_t>((state.level * 12) / 100));
```

## Multi-Screen Navigation

Register screens with `ctx.services().screen_router()` in `on_init`, then navigate:

```cpp
void on_init(blusys::app_ctx &ctx, blusys::app_services &svc, State &state)
{
    (void)svc;
    // Register screens by route ID.
    ctx.services().screen_router()->register_screen(RouteId::home,
        [](blusys::app_ctx &ctx, const void *, lv_group_t **g) -> lv_obj_t * {
            auto p = blusys::page_create();
            // ... build home screen ...
            if (g) *g = p.group;
            blusys::page_load(p);
            return p.screen;
        });

    ctx.services().navigate_to(RouteId::home);  // loads the home screen
}

// Later in update():
ctx.services().navigate_push(RouteId::settings);  // push settings screen
ctx.services().navigate_back();                   // return to home
```

## Custom Widget Contract

Product-owned custom widgets follow a lightweight contract:

| Rule | Description |
|------|-------------|
| Public `config` or `props` struct | The widget interface |
| Semantic callbacks or `ctx.dispatch(action)` | Outward behavior |
| Theme tokens only | Visual source |
| Setters own state transitions | When runtime mutation is needed |
| Standard focus and disabled model | For interactive widgets |
| Raw LVGL stays inside | Implementation detail |

### Example Custom Widget

```cpp
struct gauge_config {
    float       min   = 0.0f;
    float       max   = 100.0f;
    float       value = 0.0f;
    const char *label = "";
};

lv_obj_t *gauge_create(lv_obj_t *parent, const gauge_config &cfg);
void      gauge_set_value(lv_obj_t *gauge, float value);
```

## Bounded Raw LVGL

For advanced rendering, write raw LVGL inside a custom widget or a clearly marked block:

```cpp
// Inside a custom widget implementation or on_init:
lv_obj_t *canvas = lv_canvas_create(p.content);
lv_canvas_set_buffer(canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);
```

Raw LVGL must not:

- call `lv_screen_load()` directly (use `blusys::page_load()` or `ctx.services().navigate_to()`)
- manage UI locks directly (the framework runtime owns the lock)
- call runtime services directly from widget code
- manipulate routing or runtime internals

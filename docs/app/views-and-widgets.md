# Views & Widgets

The Blusys view layer provides stock widgets and a custom widget contract that keeps common apps off raw LVGL.

## Page Helpers

Use `page_create()` and `page_load()` to build and activate a screen:

```cpp
namespace view = blusys::app::view;

void on_init(blusys::app::app_ctx &ctx, State &state)
{
    auto p = view::page_create();  // creates screen + content col + focus group

    // Populate p.content with widgets ...

    view::page_load(p);            // activates screen and wires encoder focus
}
```

`page_create()` accepts an optional `page_config` to set padding, gap, and scrollability.

## Stock Widgets

All widget helpers take the parent container as the first argument.

### Labels and headings

```cpp
view::title(p.content, "Settings");      // large heading label
view::label(p.content, "WiFi: off");     // body label — returns lv_obj_t*

// Update text from the reducer:
lv_obj_t *lbl = view::label(p.content, "0");
// ... later in update():
view::set_text(lbl, "42");
```

### Buttons

Buttons dispatch a product action when pressed:

```cpp
view::button(p.content, "Increment",
             Action{.tag = Tag::increment}, &ctx);

// Secondary style variant:
view::button(p.content, "Reset",
             Action{.tag = Tag::reset}, &ctx,
             blusys::ui::button_variant::secondary);
```

### Slider

```cpp
view::slider(p.content, 0, 100, state.brightness,
             [](int32_t v, void *user) {
                 auto *ctx = static_cast<blusys::app::app_ctx *>(user);
                 ctx->dispatch(Action{.tag = Tag::set_brightness, .value = v});
             },
             &ctx);
```

### Toggle

```cpp
view::toggle(p.content, state.enabled,
             [](bool v, void *user) {
                 auto *ctx = static_cast<blusys::app::app_ctx *>(user);
                 ctx->dispatch(Action{.tag = Tag::set_enabled, .value = v});
             },
             &ctx);
```

### Divider and Row

```cpp
view::divider(p.content);              // horizontal separator
auto *row = view::row(p.content);      // horizontal flex container
view::button(row, "-", ...);
view::button(row, "+", ...);
```

## Multi-Screen Navigation

Register screens with `ctx.screen_router()` in `on_init`, then navigate:

```cpp
void on_init(blusys::app::app_ctx &ctx, State &state)
{
    // Register screens by route ID.
    ctx.screen_router()->register_screen(RouteId::home,
        [](blusys::app::app_ctx &ctx, const void *, lv_group_t **g) -> lv_obj_t * {
            auto p = blusys::app::view::page_create();
            // ... build home screen ...
            if (g) *g = p.group;
            blusys::app::view::page_load(p);
            return p.screen;
        });

    ctx.navigate_to(RouteId::home);  // loads the home screen
}

// Later in update():
ctx.navigate_push(RouteId::settings);  // push settings screen
ctx.navigate_back();                   // return to home
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

- call `lv_screen_load()` directly (use `view::page_load()` or `ctx.navigate_to()`)
- manage UI locks directly (the framework runtime owns the lock)
- call runtime services directly from widget code
- manipulate routing or runtime internals

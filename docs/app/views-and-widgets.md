# Views & Widgets

Use the view layer for stock widgets and small custom widgets. Keep raw LVGL inside scoped implementation blocks.

## At a glance

- `page_create()` and `page_load()` are the normal screen helpers.
- `fx.nav` registers and moves between screens.
- `primitives.hpp` gives layout building blocks.
- `widgets.hpp` gives the stock widget set.
- `action_widgets.hpp` wraps common dispatching widgets.

## Page helpers

```cpp
void on_init(blusys::app_ctx &, blusys::app_fx &, State &)
{
    auto page = blusys::page_create();
    // build page.content here
    blusys::page_load(page);
}
```

`page_create_in(shell.content_area, ...)` gives you a bounded content column inside the shell chrome.

## Navigation

Register screens in `on_init()` and move with `fx.nav.to()`, `push()`, `replace()`, `back()`, `show_overlay()`, and `hide_overlay()`.

```cpp
fx.nav.register_screen(RouteId::home, my_home_create);
fx.nav.to(RouteId::home);
```

## Stock widgets

| Category | Widgets |
|----------|---------|
| Layout | `screen`, `row`, `col`, `label`, `divider`, `icon_label`, `status_badge`, `key_value` |
| Interactive | `button`, `toggle`, `slider`, `modal`, `overlay`, `list`, `tabs`, `dropdown`, `input_field`, `knob` |
| Display | `progress`, `card`, `gauge`, `chart`, `data_table`, `level_bar`, `vu_strip` |

## Common patterns

```cpp
blusys::title(page.content, "Settings");
blusys::button(page.content, "Reset", Action{.tag = Tag::reset}, &ctx);
blusys::slider(page.content, 0, 100, state.brightness,
               [](int32_t v, void *user) {
                   auto *ctx = static_cast<blusys::app_ctx *>(user);
                   ctx->dispatch(Action{.tag = Tag::set_brightness, .value = v});
               },
               &ctx);
```

## Custom widgets

Product-owned widgets follow a small contract:

| Rule | Meaning |
|------|---------|
| Public `config` / `props` | widget interface |
| Semantic callbacks or `ctx.dispatch(action)` | outward behavior |
| Theme tokens only | visual source |
| Setters own state transitions | runtime mutation |
| Raw LVGL stays inside | implementation detail |

## Bounded raw LVGL

Use raw LVGL only inside a custom widget or a clearly marked block.

```cpp
lv_obj_t *canvas = lv_canvas_create(page.content);
lv_canvas_set_buffer(canvas, buf, w, h, LV_COLOR_FORMAT_RGB565);
```

Raw LVGL must not load screens directly, manage UI locks, or call runtime services.

## Next steps

- [Widget gallery](widget-gallery.md)
- [Profiles](profiles.md)
- [Architecture](../internals/architecture.md#ui-layout-lvgl-flex)

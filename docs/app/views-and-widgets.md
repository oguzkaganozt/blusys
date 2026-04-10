# Views & Widgets

The Blusys view layer provides stock widgets, simple bindings, and a custom widget contract that keeps common apps off raw LVGL.

## Stock Widgets

Action-bound widgets dispatch app actions directly:

```cpp
blusys::app::button("Increment", ctx, MyAction::increment);
blusys::app::button("Reset", ctx, MyAction::reset);
```

## Bindings

Simple bindings for text, value, enabled, and visible state:

```cpp
blusys::app::label(ctx, [](const MyState& s) {
    return std::to_string(s.counter);
});

blusys::app::slider(ctx, MyAction::set_brightness,
    [](const MyState& s) { return s.brightness; });
```

## Page Helpers

```cpp
auto home = blusys::app::page("Home", [](auto& ctx) {
    blusys::app::label(ctx, "Welcome");
    blusys::app::button("Start", ctx, MyAction::start);
});
```

## Custom Widget Contract

Product-owned custom widgets follow a lightweight contract:

| Rule | Description |
|------|-------------|
| Public `config` or `props` struct | The widget interface |
| Semantic callbacks or `dispatch(action)` | Outward behavior |
| Theme tokens only | Visual source |
| Setters own state transitions | When runtime mutation is needed |
| Standard focus and disabled model | For interactive widgets |
| Raw LVGL stays inside | Implementation detail |

### Example Custom Widget

```cpp
struct GaugeConfig {
    float min = 0.0f;
    float max = 100.0f;
    float value = 0.0f;
    const char* label = "";
};

class Gauge {
public:
    explicit Gauge(lv_obj_t* parent, const GaugeConfig& config);
    void set_value(float value);
    void set_label(const char* text);
private:
    lv_obj_t* arc_;    // raw LVGL stays inside
    lv_obj_t* label_;
};
```

## Bounded Raw LVGL

For advanced rendering needs, use explicit custom LVGL blocks:

```cpp
blusys::app::custom_view(ctx, [](lv_obj_t* parent) {
    // Raw LVGL is allowed here
    lv_obj_t* canvas = lv_canvas_create(parent);
    // ...
});
```

Raw LVGL must not:

- manage app screens directly
- manage UI locks directly
- call services directly
- manipulate routing or runtime internals

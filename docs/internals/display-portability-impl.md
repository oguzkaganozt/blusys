# Display Portability — Detailed Implementation Plan

Companion to [ADR 0003](adr/0003-display-auto-scaling.md).

## Purpose

Enable single-codebase UI portability: changing `sdkconfig` to select a different display (e.g.,
ST7735 160×128 → ILI9341 320×240) produces a correctly-proportioned UI without any UI code
changes.  A developer who builds the same app for two display sizes gets correct spacing, fonts,
shell chrome, and touch targets on both — zero manual retuning.

---

## Scope

**In scope**
- Color SPI/parallel TFT displays (ST7735, ILI9341, ILI9488, QEMU RGB)
- Framework UI widgets (spacing, radius, shadow, touch targets, font stepping)
- Shell chrome (header bar and tab bar collapse on compact density)
- Host SDL path (same `for_display()` runs on host for preview accuracy)

**Out of scope**
- OLED / mono page-buffer displays (`BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE`). Those use a
  dedicated `oled()` preset and the raw-LVGL path; auto-scaling is bypassed entirely.
- Per-frame or runtime re-scaling. All computation is one-pass at init time.
- Continuous font interpolation. Only `montserrat_14` ↔ `montserrat_20` are available;
  font stepping is discrete at a 0.75 scale threshold.

---

## Implementation Order

Dependencies flow downward. Complete each step before starting the next.

| Step | File | Depends on |
|------|------|-----------|
| 1 | `theme.hpp` — add `design_w`/`design_h` fields | — |
| 2a | `presets.cpp` — set `design_w`/`design_h` in all three presets | Step 1 |
| 2b | `presets.cpp` + `presets.hpp` — implement and declare `for_display()` | Step 2a |
| 3 | `display_constants.hpp` — new file, constexpr display tier helpers | — |
| 4 | `shell.cpp` — gate header + tab bar on `density_mode != compact` | Step 1 |
| 5 | `entry.hpp` — route all theme-setting through `for_display()` | Step 2b |
| 6 | `app_device_platform.cpp` + `app_host_platform.cpp` — remove dead `*_set_default_theme()` | Step 5 |

---

## Step 1 — `theme.hpp`: Add design resolution fields

**File:** `components/blusys/include/blusys/framework/ui/style/theme.hpp`

`theme_tokens` needs two new fields declaring the pixel dimensions the theme was authored for.
`for_display()` divides the actual display size by these values to compute the scale factor,
making bidirectional scaling correct (small→large and large→small both work).

**IMPORTANT — designated initializer zero-init:** C++20 designated initializer aggregate-init
zero-initialises any field not listed. Adding default member initializers (`= 320`) would be
silently overridden to 0 in presets that use designated initializers. Every preset in `presets.cpp`
**must** explicitly set `design_w` and `design_h` (Step 2a).

**Change:** Insert after `theme_feedback_voice feedback_voice;` (currently line 56), before the
spacing section:

```cpp
    // ---- design resolution ----
    // The pixel dimensions this theme was authored for.
    // for_display() divides actual display dimensions by these to compute
    // the scale factor (scale = min(actual_w/design_w, actual_h/design_h)).
    // Must be set explicitly in every preset — designated initializer
    // aggregate-init zero-initialises fields not listed, so there is no
    // safe default. A zero value disables scaling (for_display() returns
    // the theme unchanged when design_w == 0 || design_h == 0).
    std::uint32_t design_w = 0;
    std::uint32_t design_h = 0;
```

---

## Step 2a — `presets.cpp`: Set design dimensions in every preset

**File:** `components/blusys/src/framework/ui/style/presets.cpp`

Each preset must explicitly declare the resolution it was authored for.

### `expressive_dark()` — ILI9341 reference (320×240)

After `.feedback_voice = blusys::theme_feedback_voice::expressive,` add:

```cpp
        // design resolution
        .design_w = 320,
        .design_h = 240,
```

### `operational_light()` — ST7735 reference (160×128)

After `.feedback_voice = blusys::theme_feedback_voice::operational,` add:

```cpp
        // design resolution
        .design_w = 160,
        .design_h = 128,
```

### `oled()` — SSD1306 reference (128×64)

After `.feedback_voice = blusys::theme_feedback_voice::operational,` add:

```cpp
        // design resolution — OLED is excluded from auto-scaling,
        // but the dimensions must be set to prevent division-by-zero
        // if for_display() is ever called with MONO_PAGE kind.
        .design_w = 128,
        .design_h = 64,
```

---

## Step 2b — `presets.cpp` + `presets.hpp`: Implement `for_display()`

### `presets.hpp` — Add declarations and `display.h` include

**File:** `components/blusys/include/blusys/framework/ui/style/presets.hpp`

**Before:**
```cpp
#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/theme.hpp"

namespace blusys::presets {

// Expressive dark — ...
const blusys::theme_tokens &expressive_dark();
// ...
const blusys::theme_tokens &oled();

}  // namespace blusys::presets
```

**After:**
```cpp
#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/drivers/display.h"

#include <cstdint>

namespace blusys::presets {

// Expressive dark — saturated, tactile, characterful.
const blusys::theme_tokens &expressive_dark();

// Operational light — clean, readable, professionally restrained.
const blusys::theme_tokens &operational_light();

// OLED — pure white-on-black, zero radii, ultra-compact.
const blusys::theme_tokens &oled();

// Scale a developer-supplied base theme for the given display dimensions.
// Preserves all colours and non-pixel fields; adjusts only spacing, radius,
// shadow, touch_target_min, fonts, and density_mode.
// Returns a scaled copy — does not mutate `base`.
//
// Pass `profile.ui.panel_kind` as `kind`.
// When kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE the function returns
// `base` unchanged (OLED path is excluded from auto-scaling).
// When base.design_w == 0 || base.design_h == 0 the function returns
// `base` unchanged (safe no-op for uninitialised custom themes).
blusys::theme_tokens for_display(const blusys::theme_tokens &base,
                                  std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys_display_panel_kind_t kind);

// Auto-select the closest built-in base preset for the given display, then
// scale it. Equivalent to for_display(auto_base(w, h, kind), w, h, kind).
//
// Base selection:
//   MONO_PAGE       → oled()
//   short-edge ≤ 200 → operational_light()
//   short-edge > 200 → expressive_dark()
blusys::theme_tokens for_display(std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys_display_panel_kind_t kind);

}  // namespace blusys::presets

#endif  // BLUSYS_FRAMEWORK_HAS_UI
```

### `presets.cpp` — Implement `for_display()`

**File:** `components/blusys/src/framework/ui/style/presets.cpp`

Add the following after the `oled()` definition, before the closing `}  // namespace blusys::presets`:

```cpp
// ---- display-aware scaling ----

namespace {

// Scale a non-negative pixel token by ratio, floor at 1px.
// Preserves zero and negative values (e.g. radius_button = 999 is a
// sentinel meaning "pill" — scaling it preserves that intent).
inline int scale_px(int token, float ratio) noexcept
{
    if (token <= 0) {
        return token;
    }
    const int v = static_cast<int>(static_cast<float>(token) * ratio + 0.5f);
    return v < 1 ? 1 : v;
}

// Step fonts between the two available sizes.
// Below 0.75: use the smaller font.  At or above 0.75: keep the larger font.
// Non-Montserrat fonts (custom themes) are passed through unchanged.
inline const lv_font_t *scale_font(const lv_font_t *f, float ratio) noexcept
{
    const lv_font_t *sm = blusys::fonts::montserrat_14();
    const lv_font_t *lg = blusys::fonts::montserrat_20();
    if (f == sm || f == lg) {
        return (ratio < 0.75f) ? sm : lg;
    }
    return f;   // custom font — leave unchanged
}

// Auto-select the closest built-in base preset for a color display.
const blusys::theme_tokens &auto_base(std::uint32_t w, std::uint32_t h,
                                       blusys_display_panel_kind_t kind) noexcept
{
    if (kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE) {
        return oled();
    }
    const std::uint32_t short_edge = (w < h) ? w : h;
    return (short_edge <= 200) ? operational_light() : expressive_dark();
}

}  // namespace

blusys::theme_tokens for_display(const blusys::theme_tokens &base,
                                  std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys_display_panel_kind_t kind)
{
    // MONO_PAGE: return unchanged — oled preset is already pixel-perfect.
    if (kind == BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE) {
        return base;
    }

    // Guard against uninitialised design dimensions.
    // Returns unchanged rather than dividing by zero.
    if (base.design_w == 0 || base.design_h == 0) {
        return base;
    }

    const float sx    = static_cast<float>(actual_w) / static_cast<float>(base.design_w);
    const float sy    = static_cast<float>(actual_h) / static_cast<float>(base.design_h);
    const float ratio = (sx < sy) ? sx : sy;

    blusys::theme_tokens t = base;   // copy — preserves colours and all non-pixel fields

    // --- pixel-unit tokens ---
    t.spacing_xs       = scale_px(base.spacing_xs,       ratio);
    t.spacing_sm       = scale_px(base.spacing_sm,       ratio);
    t.spacing_md       = scale_px(base.spacing_md,       ratio);
    t.spacing_lg       = scale_px(base.spacing_lg,       ratio);
    t.spacing_xl       = scale_px(base.spacing_xl,       ratio);
    t.spacing_2xl      = scale_px(base.spacing_2xl,      ratio);
    t.touch_target_min = scale_px(base.touch_target_min, ratio);
    t.radius_card      = scale_px(base.radius_card,      ratio);
    t.radius_button    = scale_px(base.radius_button,    ratio);

    t.shadow_card_width   = static_cast<std::uint8_t>(scale_px(
                                static_cast<int>(base.shadow_card_width),   ratio));
    t.shadow_card_ofs_y   = static_cast<std::int16_t>(
                                static_cast<float>(base.shadow_card_ofs_y)  * ratio);
    t.shadow_card_spread  = static_cast<std::uint8_t>(scale_px(
                                static_cast<int>(base.shadow_card_spread),  ratio));
    t.shadow_overlay_width = static_cast<std::uint8_t>(scale_px(
                                static_cast<int>(base.shadow_overlay_width), ratio));
    t.shadow_overlay_ofs_y = static_cast<std::int16_t>(
                                static_cast<float>(base.shadow_overlay_ofs_y) * ratio);
    t.focus_ring_width = scale_px(base.focus_ring_width, ratio);

    // --- font stepping ---
    t.font_body    = scale_font(base.font_body,    ratio);
    t.font_body_sm = scale_font(base.font_body_sm, ratio);
    t.font_title   = scale_font(base.font_title,   ratio);
    t.font_display = scale_font(base.font_display, ratio);
    t.font_label   = scale_font(base.font_label,   ratio);
    t.font_mono    = scale_font(base.font_mono,    ratio);

    // --- density_mode from scale ---
    // compact:     scale < 0.6  (drives shell chrome collapse in shell_create)
    // normal:      0.6 ≤ scale ≤ 1.2
    // comfortable: scale > 1.2  (theme designed for a smaller display, shown on a larger one)
    if (ratio < 0.6f) {
        t.density_mode = blusys::density::compact;
    } else if (ratio > 1.2f) {
        t.density_mode = blusys::density::comfortable;
    } else {
        t.density_mode = blusys::density::normal;
    }

    // Update design dims so the returned token reflects the actual display.
    // Makes double-scaling idempotent (for_display applied twice gives the same result).
    t.design_w = actual_w;
    t.design_h = actual_h;

    return t;
}

blusys::theme_tokens for_display(std::uint32_t actual_w,
                                  std::uint32_t actual_h,
                                  blusys_display_panel_kind_t kind)
{
    return for_display(auto_base(actual_w, actual_h, kind), actual_w, actual_h, kind);
}
```

---

## Step 3 — `display_constants.hpp` (new file)

**File:** `components/blusys/include/blusys/framework/platform/display_constants.hpp`

This header provides compile-time display tier constants for `if constexpr` structural layout
branching. It is a separate header that developers include explicitly; it is not pulled in by
`widgets.hpp` or the entry macros.

Coverage: `BLUSYS_APP_DASHBOARD` Kconfig profiles only. Developers using `BLUSYS_APP_MAIN_DEVICE`
with custom profiles call `blusys::tier_for(w, h)` directly.

```cpp
#pragma once

// Compile-time display tier helpers for if constexpr structural layout branching.
//
// Usage (BLUSYS_APP_DASHBOARD builds):
//
//   #include "blusys/framework/platform/display_constants.hpp"
//
//   if constexpr (blusys::kDisplayTier == blusys::display_tier::compact) {
//       // compact layout: single column, no sidebar, abbreviated labels
//   } else {
//       // full layout: multi-column, full labels
//   }
//
// kDisplayTier, kDisplayWidth, kDisplayHeight are defined only when ESP_PLATFORM
// is set. On the host SDL path the display dimensions are set at runtime; use
// the auto_host_profile dimensions directly for host-only sizing.
//
// For BLUSYS_APP_MAIN_DEVICE with a custom profile, use tier_for(w, h) directly:
//   if constexpr (blusys::tier_for(MY_W, MY_H) == blusys::display_tier::compact)

#include <cstdint>

namespace blusys {

// Two-tier model for color displays:
//   compact — short-edge ≤ 200 px (ST7735 160×128 and smaller)
//   full    — short-edge > 200 px (ILI9341 320×240, ILI9488 480×320, etc.)
//
// OLED / mono displays are excluded from this tier model entirely;
// they use the oled() preset and the raw-LVGL path.
enum class display_tier : std::uint8_t {
    compact,
    full,
};

// Returns the display_tier for any pixel dimension pair.
// Usable in constexpr contexts for any profile.
constexpr display_tier tier_for(std::uint32_t w, std::uint32_t h) noexcept
{
    return ((w < h ? w : h) <= 200) ? display_tier::compact : display_tier::full;
}

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"

// Compile-time display dimensions from the active Kconfig dashboard profile.
// These mirror the dimensions in auto.hpp / auto_profile_dashboard().

#if defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488
inline constexpr std::uint32_t kDisplayWidth  = 480;
inline constexpr std::uint32_t kDisplayHeight = 320;
#elif defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735
inline constexpr std::uint32_t kDisplayWidth  = 160;
inline constexpr std::uint32_t kDisplayHeight = 128;
#elif defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
inline constexpr std::uint32_t kDisplayWidth  = 128;
inline constexpr std::uint32_t kDisplayHeight = 64;
#else  // default: ILI9341 320×240
inline constexpr std::uint32_t kDisplayWidth  = 320;
inline constexpr std::uint32_t kDisplayHeight = 240;
#endif

// Compile-time tier for if constexpr layout branching.
inline constexpr display_tier kDisplayTier = tier_for(kDisplayWidth, kDisplayHeight);

#endif  // ESP_PLATFORM

}  // namespace blusys
```

---

## Step 4 — `shell.cpp`: Gate chrome on `density_mode`

**File:** `components/blusys/src/framework/ui/composition/shell.cpp`

`density_mode` is set by `for_display()` based on the scale ratio but currently read nowhere in the
framework. `shell_create()` must honor it. When `density_mode == compact`, the header bar and tab
bar are suppressed. The content area already uses `lv_obj_set_flex_grow(content_area, 1)` so it
expands to fill the full screen automatically.

**Change:** In `shell_create()`, add `full_chrome` immediately before the `// Header bar.` comment
(currently line 67), and gate the two conditional blocks on it.

**Before (lines 55–68 in shell.cpp):**
```cpp
shell shell_create(const shell_config &config)
{
    const auto &t = blusys::theme();
    shell s{};

    // Root screen — non-scrollable, column layout.
    s.root = blusys::screen_create({.scrollable = false});
    lv_obj_set_flex_flow(s.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s.root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(s.root, 0, 0);
    lv_obj_set_style_pad_gap(s.root, 0, 0);

    // Header bar.
    if (config.header.enabled) {
```

**After:**
```cpp
shell shell_create(const shell_config &config)
{
    const auto &t = blusys::theme();
    shell s{};

    // Root screen — non-scrollable, column layout.
    s.root = blusys::screen_create({.scrollable = false});
    lv_obj_set_flex_flow(s.root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s.root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(s.root, 0, 0);
    lv_obj_set_style_pad_gap(s.root, 0, 0);

    // Suppress header and tab bar on compact-density themes.
    // for_display() sets density_mode = compact when scale < 0.6.
    // The content area uses flex_grow=1 and fills the full screen automatically
    // when chrome is absent — no app-side adjustment required.
    const bool full_chrome = (t.density_mode != blusys::density::compact);

    // Header bar.
    if (full_chrome && config.header.enabled) {
```

**Before (lines 134–135 in shell.cpp):**
```cpp
    // Tab bar (items added via shell_set_tabs).
    if (config.tabs.enabled) {
```

**After:**
```cpp
    // Tab bar (items added via shell_set_tabs).
    if (full_chrome && config.tabs.enabled) {
```

---

## Step 5 — `entry.hpp`: Route all theme-setting through `for_display()`

**File:** `components/blusys/include/blusys/framework/app/entry.hpp`

Both `run_host()` and `run_device()` currently pass the resolved theme raw to the platform layer.
After this change they always route through `for_display()`, which:
- scales a developer-supplied theme if `spec.theme` (or `identity->theme`) is set, preserving colours
- auto-selects and scales the closest built-in preset otherwise

`scaled_theme` is a local value. Both `platform::device_set_theme()` and `platform::host_set_theme()`
call `blusys::set_theme()` which copies the tokens into a static before returning, so passing a
pointer to a local is safe.

### Add `presets.hpp` include

Inside the `#if defined(BLUSYS_FRAMEWORK_HAS_UI)` block (around line 50) add:

```cpp
#include "blusys/framework/ui/style/presets.hpp"
```

### `run_host()` — replace lines 122–127

**Before:**
```cpp
    const blusys::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    if (resolved_theme != nullptr) {
        platform::host_set_theme(static_cast<const void *>(resolved_theme));
    } else {
        platform::host_set_default_theme();
    }
```

**After:**
```cpp
    const blusys::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    const blusys::theme_tokens scaled_theme =
        (resolved_theme != nullptr)
            ? blusys::presets::for_display(*resolved_theme,
                  static_cast<std::uint32_t>(hor_res),
                  static_cast<std::uint32_t>(ver_res),
                  BLUSYS_DISPLAY_PANEL_KIND_RGB565)
            : blusys::presets::for_display(
                  static_cast<std::uint32_t>(hor_res),
                  static_cast<std::uint32_t>(ver_res),
                  BLUSYS_DISPLAY_PANEL_KIND_RGB565);
    platform::host_set_theme(static_cast<const void *>(&scaled_theme));
```

### `run_device()` — replace lines 193–198

**Before:**
```cpp
    const blusys::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    if (resolved_theme != nullptr) {
        platform::device_set_theme(static_cast<const void *>(resolved_theme));
    } else {
        platform::device_set_default_theme();
    }
```

**After:**
```cpp
    const blusys::theme_tokens *resolved_theme = resolve_theme_tokens(spec);
    const blusys::theme_tokens scaled_theme =
        (resolved_theme != nullptr)
            ? blusys::presets::for_display(*resolved_theme,
                  static_cast<std::uint32_t>(profile.lcd.width),
                  static_cast<std::uint32_t>(profile.lcd.height),
                  profile.ui.panel_kind)
            : blusys::presets::for_display(
                  static_cast<std::uint32_t>(profile.lcd.width),
                  static_cast<std::uint32_t>(profile.lcd.height),
                  profile.ui.panel_kind);
    platform::device_set_theme(static_cast<const void *>(&scaled_theme));
```

---

## Step 6 — Remove dead `*_set_default_theme()` functions

After Step 5, `platform::device_set_default_theme()` and `platform::host_set_default_theme()` are
no longer called anywhere. Remove them.

### `entry.hpp` — remove declarations (lines 74 and 83)

**Remove:**
```cpp
void host_set_default_theme();
```
```cpp
void device_set_default_theme();
```

### `app_device_platform.cpp` — remove implementation (lines 94–97)

**Remove:**
```cpp
void device_set_default_theme()
{
    blusys::set_theme(blusys::presets::expressive_dark());
}
```

### `scripts/host/src/app_host_platform.cpp` — remove implementation (lines 225–228)

**Remove:**
```cpp
void host_set_default_theme()
{
    blusys::ui::set_theme(blusys::app::presets::expressive_dark());
}
```

---

## Scale factor reference table

The table below shows expected `for_display()` output for common display + base theme combinations.
`scale = min(actual_w/design_w, actual_h/design_h)`.

| Actual display | Base preset | design | scale | density_mode | Notes |
|---|---|---|---|---|---|
| 320×240 | auto (`expressive_dark`) | 320×240 | 1.00 | normal | No change |
| 480×320 | auto (`expressive_dark`) | 320×240 | 1.33 | comfortable | Larger spacing |
| 160×128 | auto (`operational_light`) | 160×128 | 1.00 | normal | No change |
| 320×240 | `operational_light` explicit | 160×128 | 1.87 | comfortable | Small preset on large screen |
| 160×128 | `expressive_dark` explicit | 320×240 | 0.50 | compact | Large preset on small screen |
| 128×64 | MONO_PAGE | — | — | — | Bypassed; oled() returned unchanged |

---

## Developer usage after implementation

### Zero-change case (most products)

No app code changes needed. `run_device()` auto-routes through `for_display()`.

```cpp
// Same code, correct UI on both ST7735 (160×128) and ILI9341 (320×240).
// Only sdkconfig changes between builds.
BLUSYS_APP_DASHBOARD(spec, "My Product");
```

### Custom base theme

Developer's colour/style choices are preserved; only size tokens change.

```cpp
blusys::theme_tokens my_theme = blusys::app::presets::expressive_dark();
my_theme.color_primary = lv_color_hex(0xFF6600);  // custom brand colour
// Pass via spec.theme — for_display() scales it for the actual display.
spec.theme = &my_theme;
BLUSYS_APP_MAIN_DEVICE(spec, profile);
```

### Structural layout branching

For layouts that cannot be proportionally scaled (3-column → 1-column):

```cpp
#include "blusys/framework/platform/display_constants.hpp"

lv_obj_t *build_panel(lv_obj_t *parent)
{
    if constexpr (blusys::kDisplayTier == blusys::display_tier::compact) {
        return build_panel_compact(parent);   // single column
    } else {
        return build_panel_full(parent);      // multi-column
    }
}
```

---

## Testing guidance

### Build-level (no hardware)

1. `idf.py build` with `CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9341` — baseline.
2. `idf.py build` with `CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735` — compact path.
3. `idf.py build` with `CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488` — comfortable path.
4. Host SDL build (`cmake` in `scripts/host/`) — verifies `run_host()` path compiles.

### Functional checks on hardware

1. **ST7735 (160×128):** Shell should show no header or tab bar. Content fills full screen.
2. **ILI9341 (320×240):** Shell shows header and tab bar. Spacing matches `expressive_dark` at 1:1.
3. **ILI9488 (480×320):** Shell shows header and tab bar. Spacing is visibly larger than ILI9341.
4. **Custom theme on ST7735:** Set `spec.theme = &my_custom_dark`. Verify colours preserved, sizing compact.

### Unit-level (host, no LCD)

A lightweight check that `for_display()` returns correct scaled values:

```cpp
// Verify scale factor application — 320×240 base on 160×128 display → 0.5x
auto t = blusys::presets::for_display(blusys::presets::expressive_dark(),
                                       160, 128, BLUSYS_DISPLAY_PANEL_KIND_RGB565);
assert(t.density_mode == blusys::density::compact);
assert(t.spacing_md == 6);  // 12 * 0.5 = 6
assert(t.design_w == 160 && t.design_h == 128);

// Verify MONO_PAGE bypass
auto oled_t = blusys::presets::for_display(blusys::presets::expressive_dark(),
                                            128, 64, BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE);
assert(oled_t.design_w == 320);  // unchanged — base returned as-is
```

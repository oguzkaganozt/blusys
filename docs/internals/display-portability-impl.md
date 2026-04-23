# Display Portability — Detailed Implementation Plan

Companion to [ADR 0003](adr/0003-display-auto-scaling.md).

## At a glance

- **Goal:** one product codebase; change display / `sdkconfig` and get proportionally scaled UI (widgets, shell, touch targets) without rewriting screens — [ADR 0003](adr/0003-display-auto-scaling.md) for the decision.
- **How to work:** follow **Step 1 → Step 6** in order; the **Implementation Order** table is the checklist.
- **OLED / `MONO_PAGE`:** out of scope here (separate `oled()` path); listed under **Out of scope** below.

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
| 1 | `theme.hpp` — add `design_w`/`design_h`, reorder `density` enum, document `density_mode` as derived | — |
| 2a | `presets.cpp` — add `design_w`/`design_h`, **remove** `density_mode` from all four presets | Step 1 |
| 2b | `presets.cpp` + `presets.hpp` — implement and declare `for_display()` | Step 2a |
| 3a | `dashboard_display_dims.hpp` — new file, single-source Kconfig/host → dimensions dispatch | — |
| 3b | `auto.hpp` — replace host-path literals with macros from Step 3a | Step 3a |
| 3c | `display_constants.hpp` — new file, consumes Step 3a macros (no dispatch duplication) | Step 3a |
| 4 | `shell.cpp` — gate header, status bar, and tab bar on `density_mode != compact` | Step 1 |
| 5 | `entry.hpp` — route all theme-setting through `for_display()` | Step 2b |
| 6 | `app_device_platform.cpp` + `app_host_platform.cpp` — remove dead `*_set_default_theme()` | Step 5 |

---

## Step 1 — `theme.hpp`: Design resolution + derived density

**File:** `components/blusys/include/blusys/framework/ui/style/theme.hpp`

Three changes, grouped here because all are type-level and are consumed by every later step.

**IMPORTANT — designated initializer zero-init:** C++20 designated initializer aggregate-init
zero-initialises any field not listed. Adding default member initializers (`= 320`) would be
silently overridden to 0 in presets that use designated initializers. Every preset in `presets.cpp`
**must** explicitly set `design_w` and `design_h` (Step 2a). The same constraint applies in reverse
to `density_mode`: removing it from preset initializers (Step 2a) leaves it zero-initialised, so
the enum ordering in change 1b below matters.

### 1a — Add design resolution fields

Insert after `theme_feedback_voice feedback_voice;` (currently line 56), before the spacing section:

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

### 1b — Reorder the `density` enum so `normal` is the zero value

After Step 2a removes `density_mode` from preset initializers, any theme bypassing `for_display()` receives zero-initialised `density_mode`. `normal` must be the zero value to avoid unintended chrome collapse on those paths. No code serializes the underlying integer; no code compares `density_mode` against an explicit value except `for_display()` and the Step 4 `!= compact` check.

Replace the existing enum (currently lines 13–17):

```cpp
enum class density : std::uint8_t {
    normal,       // default — balanced for most products (zero-init value)
    compact,      // industrial panels, small displays, dense status surfaces
    comfortable,  // consumer controllers, touch-first surfaces
};
```

### 1c — Document `density_mode` as derived

`for_display()` is the sole writer of `density_mode` after Step 2a. Update the docstring so preset authors don't reintroduce direct writes.

Replace the existing `// ---- density ----` block (currently lines 51–52):

```cpp
    // ---- density ----
    // Computed by presets::for_display() from the scale ratio; not set by
    // preset initializers. Direct callers that bypass for_display() receive
    // the zero-init value (density::normal). See ADR 0003.
    density density_mode;
```

---

## Step 2a — `presets.cpp`: Set design dimensions, remove `density_mode`

**File:** `components/blusys/src/framework/ui/style/presets.cpp`

Each preset must explicitly declare the resolution it was authored for, and must **stop**
setting `density_mode` — `for_display()` is the sole authority for that field after this change
(see Step 1c). Presets that still set `density_mode` would silently have their value overwritten
on the production path, making the initializer misleading.

**Common change to all four presets:** remove the `// density — …` comment and the
`.density_mode = blusys::density::…,` line from the designated-initializer list.

### `expressive_dark()` — ILI9341 reference (320×240)

1. **Remove** (currently lines 38–39):
   ```cpp
           // density — comfortable for consumer / controller
           .density_mode = blusys::density::comfortable,
   ```
2. **Add** after `.feedback_voice = blusys::theme_feedback_voice::expressive,`:
   ```cpp
           // design resolution
           .design_w = 320,
           .design_h = 240,
   ```

### `operational_light()` — ST7735 reference (160×128)

1. **Remove** the `// density — …` comment and `.density_mode = blusys::density::compact,`
   line (currently around line 128).
2. **Add** after `.feedback_voice = blusys::theme_feedback_voice::operational,`:
    ```cpp
            // design resolution
            .design_w = 160,
            .design_h = 128,
    ```

### `compact_dark()` — dark compact reference (160×128)

1. Start from `expressive_dark()` to keep the dark palette.
2. Override spacing, radii, font ramp, and motion to match the compact layout used by `operational_light()`.
3. Keep `feedback_voice = blusys::theme_feedback_voice::operational` so the compact family shares the same feedback voice.

### `oled()` — SSD1306 reference (128×64)

1. **Remove** the `// density — …` comment and `.density_mode = blusys::density::compact,`
   line (currently around line 217).
2. **Add** after `.feedback_voice = blusys::theme_feedback_voice::operational,`:
   ```cpp
           // design resolution — OLED is excluded from auto-scaling,
           // but the dimensions must be set to prevent division-by-zero
           // if for_display() is ever called with MONO_PAGE kind.
           .design_w = 128,
           .design_h = 64,
   ```

### Update the smoke assertion

`presets.cpp:16` compared `expressive.density_mode == operational.density_mode`; both are now zero-initialised to `normal`, so the assertion flips. Replace with a color or `feedback_voice` comparison instead.

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

// Compact dark — dark, compact, operationally restrained.
const blusys::theme_tokens &compact_dark();

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
// The short-edge threshold (200 px) intentionally matches
// blusys::kTierBreakpointPx in display_constants.hpp — keep in sync.
// Not #included here: presets.cpp is UI layer, that header is platform layer.
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
    // This is the sole production writer of density_mode. Presets intentionally
    // do not set it (see Step 2a); the field is authored here, from the ratio.
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

## Step 3 — Single-source compile-time display dimensions

**3a** creates one authoritative header for the Kconfig/host dispatch. **3b** and **3c** consume it, eliminating the duplicate literals currently in `auto.hpp` and avoiding a third copy in `display_constants.hpp`.

### Step 3a — `dashboard_display_dims.hpp` (new file)

**File:** `components/blusys/include/blusys/framework/platform/dashboard_display_dims.hpp`

```cpp
#pragma once

// Single-source dashboard display dimensions, dispatched from Kconfig (ESP)
// or BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE (host SDL).
//
// Consumers:
//   - auto.hpp (host-path fallback in auto_profile_dashboard / auto_host_profile_dashboard)
//   - display_constants.hpp (kDisplayWidth / kDisplayHeight for if constexpr branching)
//
// The ESP-path profile functions (profiles::ili9341_320x240 etc.) do not consume
// these macros — their dimensions live in their function names. Reviewers must
// keep the two in sync when adding a new dashboard profile; a single new profile
// requires editing exactly this header plus the matching profiles::*_WxH().

#if defined(ESP_PLATFORM)
#include "sdkconfig.h"
#endif

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB) && \
    CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB
#define BLUSYS_DASHBOARD_DISPLAY_W 320
#define BLUSYS_DASHBOARD_DISPLAY_H 240
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488
#define BLUSYS_DASHBOARD_DISPLAY_W 480
#define BLUSYS_DASHBOARD_DISPLAY_H 320
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735
#define BLUSYS_DASHBOARD_DISPLAY_W 160
#define BLUSYS_DASHBOARD_DISPLAY_H 128
#elif defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
#define BLUSYS_DASHBOARD_DISPLAY_W 128
#define BLUSYS_DASHBOARD_DISPLAY_H 64
#elif defined(ESP_PLATFORM)
// Default ESP profile: ILI9341 320×240.
#define BLUSYS_DASHBOARD_DISPLAY_W 320
#define BLUSYS_DASHBOARD_DISPLAY_H 240
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && \
    (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 1)
#define BLUSYS_DASHBOARD_DISPLAY_W 480
#define BLUSYS_DASHBOARD_DISPLAY_H 320
#elif defined(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE) && \
    (BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE == 2)
#define BLUSYS_DASHBOARD_DISPLAY_W 160
#define BLUSYS_DASHBOARD_DISPLAY_H 128
#else
// Default host: 320×240.
#define BLUSYS_DASHBOARD_DISPLAY_W 320
#define BLUSYS_DASHBOARD_DISPLAY_H 240
#endif
```

### Step 3b — `auto.hpp`: consume the shared macros

**File:** `components/blusys/include/blusys/framework/platform/auto.hpp`

Both the host fallback in `auto_profile_dashboard()` (currently lines 70–80) and the full
`auto_host_profile_dashboard()` body (currently lines 92–101) hand-dispatch
`BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE` to pick 480×320 / 160×128 / 320×240. Replace that
dispatch with the macros from Step 3a.

**Add include (near the other platform includes at the top of the file):**

```cpp
#include "blusys/framework/platform/dashboard_display_dims.hpp"
```

**In `auto_profile_dashboard()` — replace the host fallback block (currently lines 70–80):**

```cpp
#else
    device_profile p{};
    p.lcd.width          = BLUSYS_DASHBOARD_DISPLAY_W;
    p.lcd.height         = BLUSYS_DASHBOARD_DISPLAY_H;
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_UI_PANEL_KIND_RGB565;
    return p;
#endif
```

**In `auto_host_profile_dashboard()` — replace the body (currently lines 92–101):**

```cpp
[[nodiscard]] inline host_profile auto_host_profile_dashboard(const char *title)
{
    host_profile hp{};
    hp.hor_res = BLUSYS_DASHBOARD_DISPLAY_W;
    hp.ver_res = BLUSYS_DASHBOARD_DISPLAY_H;
    hp.title   = title;
    return hp;
}
```

### Step 3c — `display_constants.hpp` (new file)

**File:** `components/blusys/include/blusys/framework/platform/display_constants.hpp`

This header provides compile-time display tier constants for `if constexpr` structural layout
branching. It consumes the Step 3a macros directly — no Kconfig dispatch lives here.

Coverage: `BLUSYS_APP` interactive builds only. Developers using `BLUSYS_APP`
with custom profiles call `blusys::tier_for(w, h)` directly.

```cpp
#pragma once

// Compile-time display tier helpers for if constexpr structural layout branching.
//
// Usage (BLUSYS_APP builds):
//
//   #include "blusys/framework/platform/display_constants.hpp"
//
//   if constexpr (blusys::kDisplayTier == blusys::display_tier::compact) {
//       // compact layout: single column, no sidebar, abbreviated labels
//   } else {
//       // full layout: multi-column, full labels
//   }
//
// kDisplayWidth, kDisplayHeight, and kDisplayTier are sourced from
// dashboard_display_dims.hpp (single source for the Kconfig/host dispatch).
//
// For BLUSYS_APP with a custom profile, use tier_for(w, h) directly:
//   if constexpr (blusys::tier_for(MY_W, MY_H) == blusys::display_tier::compact)

#include "blusys/framework/platform/dashboard_display_dims.hpp"

#include <cstdint>

namespace blusys {

// Short-edge breakpoint (px) separating compact-tier displays from full-tier.
// Used by both tier_for() below and presets::auto_base() in presets.cpp.
// Change here and in auto_base() together (the value is duplicated because
// presets.cpp cannot include this platform header without a layer violation).
inline constexpr std::uint32_t kTierBreakpointPx = 200;

// Two-tier model for color displays:
//   compact — short-edge ≤ kTierBreakpointPx (ST7735 160×128 and smaller)
//   full    — short-edge >  kTierBreakpointPx (ILI9341 320×240, ILI9488 480×320, etc.)
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
    return ((w < h ? w : h) <= kTierBreakpointPx) ? display_tier::compact
                                                  : display_tier::full;
}

// Compile-time dashboard dimensions (from Step 3a macros).
inline constexpr std::uint32_t kDisplayWidth  = BLUSYS_DASHBOARD_DISPLAY_W;
inline constexpr std::uint32_t kDisplayHeight = BLUSYS_DASHBOARD_DISPLAY_H;

// Compile-time tier for if constexpr layout branching.
inline constexpr display_tier kDisplayTier = tier_for(kDisplayWidth, kDisplayHeight);

}  // namespace blusys
```


---

## Step 4 — `shell.cpp`: Gate chrome on `density_mode`

**File:** `components/blusys/src/framework/ui/composition/shell.cpp`

In `shell_create()`, add `full_chrome` before the `// Header bar.` comment (line 67) and gate all three chrome blocks on it. The content area already has `flex_grow=1` so it fills the screen when chrome is absent.

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

**Before (line 103 in shell.cpp):**
```cpp
    // Status bar.
    if (config.status.enabled) {
```

**After:**
```cpp
    // Status bar.
    if (full_chrome && config.status.enabled) {
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

Both `run_host()` and `run_device()` pass `scaled_theme` (a local) to the platform set-theme call. `set_theme()` copies tokens into a static, so a local pointer is safe.

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

???+ note "Developer usage and testing (full)"

    ## Developer usage after implementation
    
    ### Zero-change case (most products)
    
    No app code changes needed. `run_device()` auto-routes through `for_display()`.
    
    ```cpp
    // Same code, correct UI on both ST7735 (160×128) and ILI9341 (320×240).
    // Only sdkconfig changes between builds.
    spec.host_title = "My Product";
    BLUSYS_APP(spec);
    ```
    
    ### Custom base theme
    
    Developer's colour/style choices are preserved; only size tokens change.
    
    ```cpp
    blusys::theme_tokens my_theme = blusys::app::presets::expressive_dark();
    my_theme.color_primary = lv_color_hex(0xFF6600);  // custom brand colour
    // Pass via spec.theme — for_display() scales it for the actual display.
    spec.theme = &my_theme;
    spec.profile = &profile;
    BLUSYS_APP(spec);
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

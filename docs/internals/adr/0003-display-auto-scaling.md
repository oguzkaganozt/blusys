# ADR 0003: Display Auto-Scaling and Portability

**Status:** Accepted  
**Date:** 2026-04-14

## Context

Blusys products ship with a specific display, configured at build time via `device_profile`. The
framework currently has four hand-tuned presets (`expressive_dark`, `operational_light`,
`compact_dark`, `oled`)
but nothing wires them to the display automatically. A developer must:

1. Manually call `set_theme(presets::expressive_dark())` for the right preset.
2. Manually adjust spacing and layout when porting UI code to a display with a different resolution.
3. Manually collapse shell chrome (header/tab bar) when switching to a smaller display.

This friction is acceptable for a single-product build but becomes a real cost for multi-SKU
products (e.g., a basic 160×128 model and a premium 320×240 model sharing 80% of their UI code).

## Decision

Introduce a **single-pass display-aware theme scaling** mechanism that runs once during framework
initialisation, after the LCD opens and before any widget is created.

### Three-layer model

**Layer 1 — Design resolution in `theme_tokens`**

Add `design_w` and `design_h` fields to `theme_tokens`. Each preset declares the resolution it was
designed for. `for_display()` uses these to compute the scale factor.

**Layer 2 — `presets::for_display()`**

A new function that:
1. Computes `scale = min(actual_w / design_w, actual_h / design_h)`.
2. Multiplies all pixel-unit tokens (spacing, radius, shadow, touch target) by `scale`.
3. Steps fonts between the available sizes (montserrat_14 ↔ montserrat_20).
4. Sets `density_mode` from the scale (drives shell chrome visibility).
5. Returns a scaled copy of the base theme — no mutation of the original.

Two overloads:
- `for_display(base, w, h, kind)` — scales a developer-supplied base theme.
- `for_display(w, h, kind)` — auto-selects the closest base, then scales.

**Layer 3 — Automatic wiring in `run_device()` / `run_host()`**

`entry.hpp` always routes theme application through `for_display()`. If `spec.theme` is set, that
theme is scaled for the target display. If not, a suitable base is selected and scaled. The
developer's colour/style choice is preserved; only the size tokens change.

**Layer 4 — Shell chrome respects `density_mode`**

`shell_create()` gates header and tab-bar creation on `t.density_mode != density::compact`. When
the scale pushes the theme into compact mode the content area expands to fill the full screen
automatically, because it already uses `lv_obj_set_flex_grow(content, 1)`.

**Layer 5 — Compile-time layout tier for `if constexpr` branching**

A new `display_constants.hpp` header exposes:
- `blusys::tier_for(w, h)` — constexpr helper usable for any profile.
- `blusys::kDisplayTier` — derived from the active Kconfig profile.
- `blusys::kDisplayWidth / kDisplayHeight` — compile-time pixel dimensions.

Developers write `if constexpr (blusys::kDisplayTier == blusys::display_tier::compact)` to produce
different layouts on different display classes. The dead branch is eliminated by the compiler.

## Consequences

### Positive

- Switching products between display sizes (ST7735 ↔ ILI9341 ↔ ILI9488) requires changing only
  `sdkconfig` — no UI code changes.
- Products that reuse UI code across SKUs get correct proportions without manual token retuning.
- Shell chrome (header/tabs) collapses automatically on compact displays — no app-side gating.
- The mechanism is zero-cost at render time: tokens are computed once, baked into LVGL style
  properties at widget construction, never recomputed during the render loop.

### Negative / Known Limitations

- **Font scaling is discrete, not continuous.** Only `montserrat_14` and `montserrat_20` are
  available. Intermediate scale values produce either 14px or 20px. Products that need finer
  font control must enable additional Montserrat sizes in `lv_conf.h`.

- **OLED (128×64 and smaller) is excluded.** Mono displays use a dedicated `oled()` preset and
  the existing `BLUSYS_DISPLAY_PANEL_KIND_MONO_PAGE` path. Framework UI widgets are not designed
  for 1bpp displays; raw LVGL is the intended path for that tier.

- **`if constexpr` layout branching requires developer participation.** Auto-scaling handles
  proportional resizing automatically. Structural layout changes (e.g., 3-column row → 1-column
  stack) cannot be inferred from dimensions alone. The developer uses `kDisplayTier` for these.

- **`display_constants.hpp` covers `BLUSYS_APP` interactive builds only.** Developers using
  `BLUSYS_APP` with custom profiles use `blusys::tier_for(w, h)` directly instead of
  `kDisplayTier`.

## Alternatives Considered

**Compile-time `#ifdef` scaling** — rejected. Requires a separate build per display. The
`device_profile` mechanism already provides a clean runtime (init-time) selection point; there is
no benefit to moving this into the preprocessor.

**LVGL `lv_display_set_dpi()`** — rejected as the primary mechanism. LVGL's DPI system only
affects widget code that explicitly calls `lv_dpx()`. No framework widget does this; retrofitting
all of them is a larger change than token scaling with no additional benefit.

**Continuous per-frame transform scaling** — rejected. Embedded targets cannot afford per-frame
coordinate transformation. The single-pass init-time approach is the correct model.

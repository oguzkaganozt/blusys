# Phase 7 exit status

Closure record for [Phase 7: Hardware Breadth And Platform Packaging](../internals/phase-7-hardware-packaging-plan.md) and the Phase 7 section in `ROADMAP.md`.

**Closed:** 2026-04-11

## Exit criteria (ROADMAP)

| Criterion | Status |
|-----------|--------|
| Reference products run across more than one profile without app rewrites | **Met** — shared `core/`; profile chosen in `integration/` + Kconfig / host CMake; see archetype examples below |
| Adding a new profile does not require new product architecture | **Met** — data-first `device_profile` factories; same reducer and `app_spec` shape |
| Display breadth follows validated product pressure, not speculative catalog | **Met** — roadmap set only; **SH1106** explicitly deferred (SSD1306 covers Phase 7 mono path until hardware requires otherwise) |

## Deliverables (ROADMAP)

| Deliverable | Status |
|-------------|--------|
| ST7789 profile | **Met** — `components/blusys_framework/include/blusys/app/profiles/st7789.hpp` |
| SSD1306 or SH1106 profile | **Met (SSD1306)** — `ssd1306.hpp`; SH1106 not implemented by design until product need |
| ILI9341 profile | **Met** — `ili9341.hpp` |
| ILI9488 profile | **Met** — `ili9488.hpp` |
| Layout helpers (density, rotation, resolution adaptation) | **Met** — `layout_surface.hpp`: `classify()`, `shell_chrome_for()`, rotation documented in-file; `suggested_theme_density()` bridges hints to `blusys::ui::density` when UI is enabled |
| Archetype-aligned packaging | **Met** — Kconfig / README matrix; `docs/app/profiles.md` |

## Validation

### Automated

- **Phase 7 device variants:** `scripts/build-phase7-variants.sh` — interactive_controller (ST7789), interactive_panel (ILI9488), gateway (ILI9488), edge_node (SSD1306 local UI). Invoked from `.github/workflows/ci.yml` and `nightly.yml` (`phase7-variants` job) for `esp32`, `esp32c3`, `esp32s3`.
- **PR example builds:** `inventory.yml` / `build-from-inventory.sh` for curated examples as configured.

### Manual (recommended per release)

Run the same product `core/` on at least **two** physical profile variants where the archetype supports it:

| Archetype | Example | Suggested pairings |
|-----------|---------|-------------------|
| Interactive controller | `examples/quickstart/interactive_controller` | ST7735 vs ST7789 (device); host SDL sizes via `BLUSYS_IC_HOST_DISPLAY_PROFILE` |
| Interactive panel | `examples/reference/interactive_panel` | ILI9341 vs ILI9488 |
| Edge node | `examples/quickstart/edge_node` | Headless vs `BLUSYS_EDGE_NODE_LOCAL_UI` (SSD1306) |
| Gateway/controller | `examples/reference/gateway` | ILI9341 vs ILI9488 (optional local operator UI path) |

Per display family, confirm: orientation, color/inversion, offsets, focus/shell readability, and that capability + reducer behavior is unchanged across profile swaps.

### Sign-off (optional)

| Check | Owner | Date |
|-------|-------|------|
| ST7789 spot-check on hardware | | |
| SSD1306 / edge local UI spot-check | | |
| ILI9341 / ILI9488 panel spot-check | | |

## SH1106 decision

**Deferred.** The roadmap allows SSD1306 *or* SH1106. The HAL enum and profiles standardize on **SSD1306** for I2C mono. Adding `BLUSYS_LCD_DRIVER_SH1106` (or equivalent) and `profiles/sh1106.hpp` is reserved for boards that are incompatible with the SSD1306 driver path — not Phase 7 closure scope.

## Intentionally deferred

- Gateway large **headless-first** vs **interactive** fork (see Phase 8 ecosystem work).
- Full theme token cloning from `surface_hints` beyond `suggested_theme_density()` — products use stock presets or custom themes as today.

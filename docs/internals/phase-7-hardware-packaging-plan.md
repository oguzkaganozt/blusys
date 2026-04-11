# Phase 7 Hardware Breadth And Platform Packaging Plan

This document turns the Phase 7 target from `ROADMAP.md` into an execution-ready implementation plan for the current repo state.

Use it together with:

- `PRD.md` for product requirements and locked decisions
- `ROADMAP.md` for sequencing, dependencies, and phase exit criteria
- `docs/internals/architecture.md` for tier and ownership constraints
- `docs/internals/guidelines.md` for support-tier and validation rules
- `docs/internals/phase-5-interactive-archetypes-plan.md` for the interactive reference pressure that should drive hardware breadth
- `docs/internals/phase-6-connected-archetypes-plan.md` for the connected reference pressure that should drive optional local-surface breadth

## Implementation status

This section tracks progress against this plan in the repo. Update it when workstreams land or scope shifts.

### Done

- **HAL — public driver IDs (`components/blusys/include/blusys/drivers/display/lcd.h`)**  
  - `BLUSYS_LCD_DRIVER_ILI9341` and `BLUSYS_LCD_DRIVER_ILI9488` appended to the enum (existing values `0`–`3` unchanged).

- **HAL — ILI9341 / ILI9488 SPI panels (`components/blusys/src/drivers/display/lcd_panel_ili.c`, `components/blusys/include/blusys/internal/lcd_panel_ili.h`)**  
  - ESP-IDF 5.5 `esp_lcd` does **not** provide `esp_lcd_panel_ili9341` / `esp_lcd_panel_ili9488` headers; Blusys implements in-tree factories compatible with the same `esp_lcd_panel` contract as stock ST7789 (address window + `RAMWR`).  
  - Init sequences are typical-breakout defaults; specific modules may still need profile-level tweaks (offsets, inversion, gamma).

- **HAL — `lcd.c` integration**  
  - `blusys_lcd_open_spi` selects ILI9341 / ILI9488 via the new factories; NT35510 is chosen **only** for `BLUSYS_LCD_DRIVER_NT35510` (no accidental fallback).  
  - SPI `max_transfer_sz` uses per-row bytes and caps total transfer size to avoid huge DMA reservations on large logical sizes (verify full-frame flush assumptions for very large panels).

- **Build smoke**  
  - At least one device build of a canonical interactive quickstart (e.g. `interactive_controller` / `esp32`) has been run successfully with this HAL linked in.

- **Framework — product-facing profiles (`components/blusys_framework/include/blusys/app/profiles/`)**  
  - `st7789.hpp` — `st7789_320x240()` (SPI, landscape 320×240, target-gated pins).  
  - `ili9341.hpp` — `ili9341_320x240()`.  
  - `ili9488.hpp` — `ili9488_480x320()` (conservative SPI clock, larger `buf_lines`).  
  - `ssd1306.hpp` — `ssd1306_128x64()`, `ssd1306_128x32()` (I2C, `MONO_PAGE`, pins aligned with `lcd_ssd1306_basic` defaults).

- **Framework — layout hints (`components/blusys_framework/include/blusys/app/layout_surface.hpp`)**  
  - `blusys::app::layout::classify()` — surface size class, shell density, spacing/typography levels, theme packaging hint from `device_profile` or raw width/height (not a layout engine).

- **Workstreams D–E (partial) — Kconfig + wiring**  
  - `examples/quickstart/interactive_controller` — `main/Kconfig.projbuild` choice **ST7735** (default) vs **ST7789**; `controller_device_profile_for_build()` + `layout::classify` drives shell chrome; host SDL window size via CMake cache `BLUSYS_IC_HOST_DISPLAY_PROFILE` (0 = 240×240, 1 = 320×240).  
  - `examples/reference/interactive_panel` — choice **ILI9341** (default) vs **ILI9488** (replaces prior fixed ST7735 on device); shell from `layout::classify`; host `BLUSYS_IP_HOST_DISPLAY_PROFILE` (0 = 320×240, 1 = 480×320).  
  - `examples/reference/gateway` — same ILI9341 / ILI9488 choice + layout-driven shell; WiFi options preserved in `Kconfig.projbuild`.  
  - Host CMake for these three examples adds `blusys_services/include` so `layout_surface.hpp` resolves on SDL builds.

### Remaining

- **Workstream A (remainder)**  
  - Confirm ST7789 / SSD1306 HAL paths for recommended boards; add **SH1106** only if product validation requires it beyond SSD1306 (still deferred by default).  
  - Optional: small `device_profile` extensions **only** if layout helpers need extra metadata (prefer deriving hints from existing `lcd` fields first).

- **Workstream B — product-facing profiles (follow-up)**  
  - Field-test pin/orientation defaults on real boards; adjust comments or defaults per validated modules if needed.

- **Workstream C — layout adaptation helpers (follow-up)**  
  - Optional: map `surface_hints` into theme token scaling or stock widget presets when repetition appears.

- **Workstreams D–E — references (remainder)**  
  - `edge_node` — **done**: Kconfig `BLUSYS_EDGE_NODE_LOCAL_UI` enables `BLUSYS_APP_MAIN_DEVICE` + `profiles/ssd1306_128x64()` and minimal mono status UI in `main/ui/` (same `logic/` + capabilities). Default remains headless. Project sets `BLUSYS_BUILD_UI` when the optional UI path is used.  
  - `gateway` — optional **headless** vs interactive (large product fork; still deferred).  
  - **Gateway packaging** — `partitions.csv` added (aligned with other 4 MB + SPIFFS quickstarts).  
  - Hardware proof: run the same `logic/` on at least two physical profile variants per archetype (manual).

- **Workstream F — docs and discoverability**  
  - **Done**: `docs/app/profiles.md` extended with Phase 7 profile table, layout hints, Kconfig pointers, and archetype→profile table. No new nav entries (`profiles.md` already listed in `mkdocs.yml` / `inventory.yml`).

- **Workstream G — validation**  
  - `blusys lint`, targeted `blusys host-build` / `blusys build` on supported targets, manual hardware checklist per new display family before calling a profile “recommended”.

- **Exit criteria (see below)**  
  - Not all roadmap exit criteria are met until references run on multiple profiles without app-architecture rewrites and optional connected local surface is demonstrated.

## Phase Goal

Support the additional hardware and packaging breadth that the proven `blusys::app` product model actually needs, without introducing new product architectures or speculative catalog expansion.

Phase 7 must deliver:

- `ST7789` profile
- `SSD1306` or `SH1106` profile
- `ILI9341` profile
- `ILI9488` profile
- layout helpers for density, rotation, and resolution adaptation
- archetype-aligned packaging of profiles, widgets, and flows for the four canonical starting shapes

This phase is not about building a generic display framework. It is about making the already-proven product model portable across the next set of validated hardware surfaces.

## Phase Constraints

The following decisions are already locked by `PRD.md` and `ROADMAP.md` and must not be reopened during Phase 7:

- top-level runtime modes remain `interactive` and `headless`
- archetypes stay starter compositions, not framework branches
- product-facing code stays reducer-driven: `update(ctx, state, action)`
- product-facing namespace remains `blusys::app`
- capabilities stay composed in `system/`, not in `logic/` or `ui/`
- scaffold and product structure stay `main/`, `logic/`, `ui/`, and `system/`
- raw LVGL is allowed only inside custom widget implementations or explicit bounded custom view scope
- the three-tier architecture stays intact; display-controller support belongs in HAL or services, while profiles and packaging belong in the framework
- hardware breadth must follow validated product pressure rather than speculative board-catalog growth

## Current Repo Baseline

The current repo has enough product-model maturity to plan Phase 7, but not enough hardware breadth to consider the packaging problem solved.

What already exists:

- `blusys::app` runtime, reducer model, profiles, and entry macros are real
- the current product-facing profile surface already includes `host`, `headless`, and the canonical `st7735` device profile
- the current `device_profile` already centralizes framework-owned LCD, UI, encoder, button, and brightness bring-up
- the shell, widget, theme, capability, and flow layers already exist from the Phase 0 through Phase 6 foundation
- the HAL LCD layer already exposes `ST7789`, `SSD1306`, `NT35510`, and `ST7735` driver identifiers; **`ILI9341` and `ILI9488` are now public enum values with in-tree SPI panel support** (see **Implementation status**)
- the UI service already supports both RGB565-class panels and mono page-format panels

What is still missing or incomplete:

- product-facing profiles do not yet exist for `ST7789`, `SSD1306` or `SH1106`, `ILI9341`, or `ILI9488` (**HAL drivers for ILI9341/ILI9488 are in place; framework `profiles/*.hpp` not yet added**)
- the lower-tier LCD enum does not yet expose **`SH1106`** as a public driver choice (optional; prefer SSD1306 unless hardware forces otherwise)
- there is no small app-facing layout adaptation layer for density, orientation, and resolution changes
- archetype packaging is still implicit in examples and reference apps rather than formalized as recommended profile and composition guidance

This means Phase 7 is partly framework packaging work and partly prerequisite lower-tier enablement work.

## Phase Deliverables

Phase 7 should produce the following outputs:

1. A stable product-facing profile contract that can describe the next validated display families without leaking low-level panel bring-up into app code.
2. Product-facing profiles for the next supported display families.
3. Small framework-owned layout helpers for density, rotation, and resolution adaptation.
4. Archetype-aligned packaging guidance for profiles, widgets, themes, capabilities, and flows.
5. Validation proving that reference products run across more than one profile without app rewrites.

## Scope Lock Before Implementation

Before expanding the profile catalog, Phase 7 should lock its scope against the product pressure established by Phase 5 and Phase 6.

The phase should explicitly answer:

- which Phase 5 interactive references need to run across more than one display profile
- which Phase 6 connected references need an optional local UI surface beyond the current canonical hardware
- which display classes are actually needed: compact TFT, tiny mono status surface, medium panel, large panel
- which input expectations matter for each display family: encoder-first, touch-ready, button-array friendly, or status-only

This prevents the phase from turning into a speculative controller catalog.

## Workstreams

### Workstream A: Profile Contract And Lower-Tier Gap Closure

The current `device_profile` contract is deliberately small and should stay product-facing and direct.

Phase 7 should extend it only where reusable profile breadth actually requires it.

Required outcomes:

- keep `device_profile` as the single recommended device entry surface for interactive products
- add only the minimum extra profile metadata needed for reusable layout adaptation and packaging
- avoid forcing product code to manage raw controller quirks such as register details, flush format differences, or controller-specific init behavior
- close lower-tier LCD driver gaps required for the selected profile set

Expected lower-tier work:

- verify the existing `ST7789` and `SSD1306` lower-tier paths are sufficient for recommended product use
- add public lower-tier support for `ILI9341` if it is not already available behind the current LCD abstraction
- add public lower-tier support for `ILI9488` if it is not already available behind the current LCD abstraction
- add `SH1106` only if validated hardware pressure requires it beyond the existing `SSD1306` path

Decision rule:

- prefer `SSD1306` as the Phase 7 mono-profile target unless real product pressure requires `SH1106`

### Workstream B: Product-Facing Profile Expansion

Add the new `blusys::app` profile headers in a way that mirrors the current `st7735` profile: small, data-first, and easy to override.

Recommended profile set:

- `st7789.hpp`
- `ssd1306.hpp`
- `ili9341.hpp`
- `ili9488.hpp`

Each profile should provide:

- one obvious default constructor function
- panel dimensions and pixel-format defaults
- orientation defaults that match the recommended starter use
- target-gated pin defaults only where they are trustworthy and common enough to reduce setup burden
- UI panel-kind defaults that match the actual display family
- comments showing the product-owned fields that callers are expected to override

Profile intent should stay explicit:

- `ST7789` for compact-to-medium interactive color devices
- `SSD1306` for tiny status or low-power local surfaces
- `ILI9341` for mainstream dashboard or operator-panel surfaces
- `ILI9488` for large local surfaces where validated product pressure justifies the memory and rendering cost

### Workstream C: Layout Adaptation Helpers

Phase 7 needs a small app-facing adaptation layer so the same product app can move across profile sizes without turning every screen into handwritten dimension logic.

Helpers should cover:

- display size classification
- orientation classification
- density recommendations tied to theme tokens
- spacing and typography adaptation for compact versus larger surfaces
- shell presentation differences such as header, status, and tab treatment
- widget selection helpers where compact and expanded surfaces need different stock compositions

Guardrails:

- do not build a general responsive-layout engine
- do not require product code to scatter raw width and height checks across screens
- keep the helper surface small, explicit, and aligned to the existing shell, widget, and screen model

Recommended output shape:

- a small `blusys::app` helper surface for screen classification and profile-derived layout hints
- examples in the archetype references showing how one app switches between compact, medium, and large surfaces with minimal code branching

### Workstream D: Archetype-Aligned Packaging

Phase 7 should package what the earlier phases have already proven.

This packaging must remain composition-based, not branch-based.

Target packaging direction:

- `interactive controller`
  - recommended profiles: `ST7735`, `ST7789`
  - default identity bias: `expressive_dark`
  - widget bias: compact control widgets, status badges, tactile control surfaces
  - flow bias: setup, settings, compact control, status
- `interactive panel`
  - recommended profiles: `ILI9341`, `ILI9488`
  - default identity bias: `operational_light`
  - widget bias: dashboard, list, tabs, diagnostics, denser data presentation
  - flow bias: dashboard, diagnostics, settings, local control
- `edge node`
  - recommended mode bias: headless first
  - optional local-surface profile: `SSD1306`
  - widget bias: minimal status and alert presentation only when a local surface exists
  - flow bias: provisioning, connectivity, telemetry, OTA, diagnostics
- `gateway/controller`
  - recommended profiles: headless by default, `ILI9341` for local operator panel, `ILI9488` only if the validated product surface needs more density
  - widget bias: status overview, diagnostics, settings, maintenance, local control
  - flow bias: orchestration, diagnostics, settings, connectivity, maintenance, operator status

Packaging should first appear as:

- docs guidance
- starter composition guidance
- example and reference conventions
- scaffold defaults in the later ecosystem and archetype-starter phase

Only promote composition helpers into public API if repeated real usage proves they reduce product-owned burden.

### Workstream E: Reference Product Portability Proof

Phase 7 is only complete if the proven reference products can actually move across the new profile set.

Required proof points:

- at least one interactive reference app runs on more than one color profile without app-architecture changes
- at least one connected reference can stay headless while also supporting an optional local surface profile without moving product behavior out of `logic/`
- profile swaps change hardware config and layout adaptation behavior, not the reducer model or capability ownership model

Recommended proof strategy:

- use the `interactive controller` reference to validate compact color portability across `ST7735` and `ST7789`
- use the `interactive panel` reference to validate medium-to-large color portability across `ILI9341` and `ILI9488`
- use the `edge node` reference to validate optional tiny local status presentation on `SSD1306`
- use the `gateway/controller` reference to validate optional operator-panel variants without changing the core connected app model

### Workstream F: Docs, Examples, And Discoverability

When the hardware breadth is credible, update the public surface so the new profiles and packaging choices are easy to discover from the recommended path.

Required changes:

- add profile documentation for each supported Phase 7 profile
- explain when to choose `ST7735` versus `ST7789`, `SSD1306` versus `SH1106` if both exist, and `ILI9341` versus `ILI9488`
- add archetype guidance mapping each of the four canonical starting shapes to recommended profile families
- update example and reference documentation when one app is validated across multiple profiles
- update `inventory.yml` and docs navigation for any new docs or example surfaces added during the phase

The documentation story should stay product-first:

- choose an archetype
- choose a runtime mode
- choose the recommended display profile if a local surface exists
- keep the same product app structure and reducer model

### Workstream G: Validation And Hardware Confidence

Phase 7 validation must prove portability, not just compile success.

Required validation:

- `blusys lint` for any layering-sensitive lower-tier display work
- targeted `blusys host-build` coverage for the reference apps at matching host-profile resolutions
- targeted `blusys build` coverage for the new profiles on the intended supported targets
- reducer and capability regression checks after profile swaps on the reference apps
- screenshot or visual smoke coverage for compact, medium, large, and mono surfaces where practical
- manual hardware smoke for at least one real board per new display family before the profile is treated as recommended

Recommended manual checklist:

- orientation defaults match the intended product layout
- color order and inversion defaults are correct
- clipping and padding behave correctly near the screen edges
- focus and input behavior still feel coherent after profile swaps
- shell layout remains readable at the smallest and largest validated resolutions
- mono status surfaces remain useful without pulling connected products into an interactive-only architecture

## Proposed Repository Landing Pattern

Phase 7 should not explode the repo into board-specific starter trees.

Recommended landing pattern:

- profiles live under `components/blusys_framework/include/blusys/app/profiles/`
- any small app-facing layout adaptation helpers live in the `blusys::app` product layer
- lower-tier controller support stays in `components/blusys/` and `components/blusys_services/` as needed
- portability proof lands in the existing archetype references from Phase 5 and Phase 6 rather than in a new matrix of controller demo apps

This keeps Phase 7 grounded in the validated product stories rather than shifting attention back to raw hardware demos.

## Milestone Order

The recommended execution order for Phase 7 is:

1. Lock the exact display-family scope against the needs exposed by the Phase 5 and Phase 6 reference products.
2. Choose the canonical mono profile target for the phase, defaulting to `SSD1306` unless real hardware pressure requires `SH1106`.
3. Close any lower-tier driver gaps needed for the selected profile set.
4. Extend the profile contract only as much as is necessary for reusable layout adaptation.
5. Land the `ST7789` profile first as the smallest next-step color breadth expansion.
6. Land the mono profile next.
7. Land `ILI9341` after the medium-panel path is validated.
8. Land `ILI9488` last, only after memory and rendering behavior are understood well enough to recommend it.
9. Add layout adaptation helpers driven by the actual reference products.
10. Update the reference products to prove multi-profile portability.
11. Publish the archetype-aligned packaging guidance and profile docs.

This order keeps new breadth downstream of proven product pressure, consistent with the roadmap dependency rules.

## Exit Criteria

Phase 7 is complete when all of the following are true:

- at least one interactive reference product runs on more than one display profile without app-architecture changes
- at least one connected reference product supports an optional local surface without changing the core connected operating model
- adding a new profile no longer implies inventing a new product structure or a new product-facing architecture
- display adaptation is handled through framework helpers and profile choices rather than repeated app-specific screen rewrites
- the four canonical archetypes all have clear recommended profile and packaging guidance
- the hardware surface remains intentionally smaller than a generic board-support catalog

## Risks To Watch

- turning Phase 7 into a broad display-controller program instead of a product-portability phase
- widening `device_profile` until it leaks HAL complexity back into product code
- making archetype packaging look like framework branching
- adding touch and input breadth speculatively rather than in response to validated product pressure
- landing large-panel support without proportionate memory, flush, and hardware validation

## Decision Rules During Phase 7

When implementation choices are ambiguous, prefer the option that:

- keeps the public API profile-based and product-facing
- pushes controller-specific complexity downward into HAL or services
- reuses the same product app across multiple profiles
- packages proven compositions rather than inventing new branches
- adds the smallest amount of adaptation machinery needed for the reference products

If a proposed profile or helper does not clearly improve one of the four canonical archetype starting shapes, it should not become part of the recommended Phase 7 surface.

# Blusys V2 Plan

Created: 2026-04-10. Baseline: v6.1.0.

## Execution Order

| # | Item | Effort | Impact |
|---|------|--------|--------|
| 1 | Dead links & doc gate cleanup | 30 min | Quick win — doc gate temiz, yeni devs takılmaz |
| 2 | Scaffold production-ready | 2-3 saat | Her yeni ürünün ilk gününü kurtarır |
| 3 | Host-side test foundation | 1-2 gün | Platform güvenilirliğinin temeli |
| 4 | Widget source auto-discovery | 1 saat | bu_knob öncesi gerekli, bakım yükü sıfırlanır |
| 5 | `bu_knob` — Camp 3 widget | 2-3 gün | V2 headline feature, sonraki widget'ları unlock'lar |
| 6 | Sanitizer builds | 1-2 saat | Slot pool / callback / lock bug'larını yakalar |
| 7 | Headless snapshot renderer | 2-3 gün | Widget kit büyüdükçe visual regression koruma |

Estimated total: ~2 weeks. Items 1-4 infrastructure, 5 feature, 6-7 hardening.

---

## Item Details

### 1. Dead Links & Doc Gate Cleanup

Three doc files reference the deleted `platform-transition/` directory and `PROGRESS.md`.
`widget-author-guide.md` has a broken relative path to `../../platform-transition/UI-system.md`.

**What to do:**
- Remove or redirect broken references in `docs/guides/framework.md`, `docs/migration-guide.md`,
  and `components/blusys_framework/widget-author-guide.md`
- Verify `mkdocs build --strict` passes

---

### 2. Scaffold Production-Ready

The scaffold is every new product's starting point. Three gaps:

**Route sink:** `runtime.init(&controller, /*output_route_sink=*/nullptr, 10)` — the overlay
chain (`show_overlay`/`hide_overlay`) does not work out of the box. Wire a default route sink
that maps route commands to widget calls (same pattern as `widget_kit_demo`'s `ui_route_sink`).

**Host encoder sim:** The interactive scaffold's `host/main.cpp` only wires mouse + keyboard
LVGL drivers but does not connect keyboard events to encoder simulation. Port the
`sdl_encoder_key_watch` + `encoder_indev_read_cb` pattern from `widget_kit_demo`.

**README template:** Scaffold a minimal `README.md` with build/flash/host commands so product
teams have a runnable cheat sheet from day one.

---

### 3. Host-Side Test Foundation

Zero unit tests today. The framework spine is pure C++ and testable on host without hardware.

**What to do:**
- Add a lightweight test framework (Catch2 single-header or similar) to `scripts/host/`
- Write tests for:
  - Slot pool acquire / release / exhaust / delete lifecycle
  - Runtime → controller → route_sink intent pipeline
  - `auto_focus_screen` DFS traversal correctness
  - Encoder indev state buffer (diff accumulation, press/release)
- Add a `ctest` target to `scripts/host/CMakeLists.txt`
- Add test step to CI

---

### 4. Widget Source Auto-Discovery

Adding a new widget currently requires manual edits in two CMakeLists files (monorepo host
harness + scaffold host harness). Both list framework widget sources explicitly.

**What to do:**
- Replace the explicit source list in `scripts/host/CMakeLists.txt` with `file(GLOB_RECURSE ...)`
  over the framework widget directories
- Apply the same pattern to the scaffold's `host/CMakeLists.txt` template
- Validate that `bu_knob` (item 5) is picked up automatically

---

### 5. `bu_knob` — Camp 3 Rotary Widget

First widget using `lv_obj_class_t` embedded storage (Camp 3) instead of the external slot
pool (Camp 2). This is not just a knob — it proves the widget system can support a second
implementation pattern and unlocks future Camp 3 widgets.

**What to do:**
- Implement `bu_knob` in `components/blusys_framework/src/ui/widgets/knob/`
- Use `lv_arc` as the underlying LVGL primitive with encoder-friendly value control
- Add Camp 3 section to `widget-author-guide.md`
- Add `knob_basic` example
- Add to host harness (`widget_kit_demo` + scaffold)
- Docs: `docs/modules/bu_knob.md` + `docs/guides/knob-basic.md`

---

### 6. Sanitizer Builds

The host harness already builds on Linux. Adding ASan + UBSan is a single CMake option.

**What to do:**
- Add `-DSANITIZE=ON` option to `scripts/host/CMakeLists.txt`
- When enabled: `target_compile_options(... -fsanitize=address,undefined)` +
  `target_link_options(... -fsanitize=address,undefined)`
- Run all host targets + tests (item 3) under sanitizers in CI

---

### 7. Headless Snapshot Renderer

`scripts/host/README.md` already mentions "per-widget visual snapshot tests" as planned.
Currently the SDL2 driver requires a display window, blocking CI use.

**What to do:**
- Add an LVGL software renderer backend that writes to an in-memory framebuffer
- Export framebuffer as PNG after rendering each widget in isolation
- Compare against checked-in reference PNGs (pixel-diff with tolerance)
- Add snapshot step to CI
- Reference PNGs live under `scripts/host/snapshots/`

---

## Explicitly Not in V2

| Item | Reason |
|------|--------|
| Services → C++ | Decision: services stay C. C++ only for framework + application layer. |
| Per-module build gating | Premature until 3+ products with different module subsets exist. |
| Sensor service placeholder | No demand yet. Empty dirs stay as-is. |
| LVGL version upgrade | v9.2.2 is stable. Review when v9.3+/v10.x stabilize. |
| macOS CI validation | Ubuntu sufficient. Add macOS when there is demand. |
| Latency tooling | Requires hardware bench session. Parked with V1.1 hardware items. |

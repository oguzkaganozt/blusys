# Blusys V2 Plan

Created: 2026-04-10. Baseline: v6.1.0.

## Execution Order

| # | Item | Effort | Impact |
|---|------|--------|--------|
| 1 | Dead links and doc gate cleanup | 30 min | Quick win; doc gate stays clean and new developers do not trip over stale references |
| 2 | Scaffold production-ready | 2-3 hours | Improves the first-day experience for every new product |
| 3 | Host-side test foundation | 1-2 days | Establishes the base for platform reliability |
| 4 | Widget source auto-discovery | 1 hour | Removes manual host-harness upkeep ahead of `bu_knob` |
| 5 | `bu_knob` - Camp 3 widget | 2-3 days | V2 headline feature that unlocks later widget work |
| 6 | Sanitizer builds | 1-2 hours | Catches slot-pool, callback, and lock bugs earlier |
| 7 | Headless snapshot renderer | 2-3 days | Adds visual regression protection as the widget kit grows |

Estimated total: about 2 weeks. Items 1-4 are infrastructure, item 5 is the feature headline, and items 6-7 are hardening.

## Item Details

### 1. Dead Links And Doc Gate Cleanup

Broken references should be removed or redirected so `mkdocs build --strict` stays reliable.

What to do:
- Remove or redirect stale references in framework and migration docs
- Verify `mkdocs build --strict` passes

### 2. Scaffold Production-Ready

The scaffold is every new product's starting point. Remaining gaps should be closed so generated projects are runnable without hand-fixing.

What to do:
- Ship a default route sink so overlay commands work out of the box
- Wire encoder simulation into the interactive host scaffold
- Generate a minimal `README.md` with build, flash, and host commands

### 3. Host-Side Test Foundation

The framework spine is host-testable and should have an actual automated test layer.

What to do:
- Add a lightweight C++ test framework to `scripts/host/`
- Cover slot-pool lifecycle, runtime-to-route-sink flow, focus traversal, and encoder state buffering
- Add a `ctest` target and run it in CI

### 4. Widget Source Auto-Discovery

Adding widgets should not require touching multiple explicit source lists.

What to do:
- Replace explicit widget source lists in the host harness with recursive discovery
- Apply the same approach to scaffold host templates
- Verify new widgets are picked up automatically

### 5. `bu_knob` - Camp 3 Rotary Widget

This is the first widget using `lv_obj_class_t` embedded storage instead of the external Camp 2 slot-pool pattern.

What to do:
- Implement `bu_knob` under `components/blusys_framework/src/ui/widgets/knob/`
- Base it on `lv_arc` with encoder-friendly value control
- Extend `widget-author-guide.md` with a Camp 3 section
- Add example, host harness coverage, and docs

### 6. Sanitizer Builds

The host harness should support ASan and UBSan as an opt-in CI hardening path.

What to do:
- Add `-DSANITIZE=ON` to the host CMake flow
- Wire the required compile and link flags
- Run host targets and tests under sanitizers in CI

### 7. Headless Snapshot Renderer

Visual regression checks should not require a windowed SDL session.

What to do:
- Add an LVGL software renderer backend that writes to an in-memory framebuffer
- Export PNGs and compare against checked-in references with tolerance
- Add snapshot checks to CI

## Explicitly Not In V2

| Item | Reason |
|------|--------|
| Services to C++ | Services stay C; C++ remains framework and application only |
| Per-module build gating | Premature until there are several distinct product shapes to justify it |
| Sensor service placeholder | No current demand |
| LVGL version upgrade | v9.2.2 is stable enough for now |
| macOS CI validation | Ubuntu is sufficient until there is real demand |
| Latency tooling | Depends on hardware bench work parked with current follow-ups |

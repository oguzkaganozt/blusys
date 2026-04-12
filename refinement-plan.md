# Blusys Codebase Refinement Plan

## Context

The Blusys codebase (~28K LOC across 3 tiers) is a well-architected ESP32 product platform with clean layering and disciplined conventions. However, a deep audit reveals **concentrated duplication in the widget layer** that can be systematically eliminated. A template helper (`detail::fixed_slot_pool.hpp`) was introduced to centralize slot pool logic, but only 2 of 10 pool-based widgets adopted it — the remaining 8 carry ~200 lines of hand-rolled duplicates. Additionally, 7 widgets duplicate an identical disabled-state setter, and the gauge widget uses a non-standard split-array pool.

**Goal:** Eliminate widget-layer duplication, normalize inconsistent patterns, and extract common helpers — all without changing any public API.

---

## Phase 1 — Architecture Summary

### Three-Tier Model

```
blusys_framework (C++, ~9.8K LOC)
        ↓
blusys_services  (C, ~8.5K LOC)
        ↓
blusys           (C HAL, ~7K LOC)
        ↓
ESP-IDF v5.5+
```

Layering enforced by `blusys lint` — no reverse dependencies allowed.

### Widget Inventory (17 stock widgets)

| Pattern | Widgets | Count |
|---------|---------|-------|
| Camp 2 — slot pool with hand-rolled helpers | button, dropdown, modal, list, data_table, input_field, tabs, overlay | 8 |
| Camp 2 — slot pool using `detail::acquire_ui_slot` | slider, toggle | 2 |
| Camp 2 — data pool with hand-rolled helpers (no callbacks) | gauge (split-array), chart, vu_strip | 3 |
| Camp 3 — per-instance `lv_malloc` | knob | 1 |
| No pool (display-only / stateless) | card, progress, level_bar | 3 |

### Key Files

| File | Role |
|------|------|
| `components/blusys_framework/include/blusys/framework/ui/detail/fixed_slot_pool.hpp` | Template helpers for slot pools |
| `components/blusys_framework/src/ui/widgets/button/button.cpp` | Canonical Camp 2 widget (template others copy) |
| `components/blusys_framework/src/ui/widgets/*/` | All 17 stock widget implementations |
| `components/blusys_framework/widget-author-guide.md` | Widget contract (6 rules) |
| `cmake/blusys_framework_ui_sources.cmake` | Widget auto-discovery via GLOB |

---

## Phase 2 — Detailed Audit

### Finding 1: Widget Slot Pool Duplication (HIGH PRIORITY)

**Issue:** 8 widgets have hand-rolled `acquire_slot()` / `release_slot()` that are functionally identical to `detail::acquire_ui_slot` / `detail::release_ui_slot`, but were never migrated.

**Root cause:** `release_ui_slot` (fixed_slot_pool.hpp:37-44) hardcodes `slot->on_change` as the callback field name. Most widgets use different names (`on_press`, `on_select`, `on_dismiss`, `on_submit`, `on_row_select`, `on_hidden`), so they couldn't use the template.

**Evidence — identical logic in every widget:**
```cpp
// Hand-rolled acquire (button.cpp:37-49, dropdown.cpp:34-46, modal.cpp:36-48, etc.)
X_slot *acquire_slot() {
    for (auto &s : g_X_slots) {
        if (!s.in_use) { s.in_use = true; return &s; }
    }
    BLUSYS_LOGE(kTag, "slot pool exhausted (size=%d) — raise BLUSYS_UI_X_POOL_SIZE", ...);
    assert(false && "...");
    return nullptr;
}

// Hand-rolled release (button.cpp:52-59, dropdown.cpp:49-56, modal.cpp:51-58, etc.)
void release_slot(X_slot *slot) {
    if (slot != nullptr) {
        slot->on_callback = nullptr;
        slot->user_data   = nullptr;
        slot->in_use      = false;
    }
}
```

**Versus the template (used only by slider and toggle):**
```cpp
X_slot *acquire_slot() {
    return detail::acquire_ui_slot(g_X_slots, kTag, "BLUSYS_UI_X_POOL_SIZE");
}
void release_slot(X_slot *slot) {
    detail::release_ui_slot(slot);
}
```

**Affected widgets with slot shapes:**

| Widget | File (lines) | Callback field | Extra fields |
|--------|-------------|---------------|--------------|
| button | `button/button.cpp:37-59` | `on_press` | — |
| dropdown | `dropdown/dropdown.cpp:34-56` | `on_select` | — |
| modal | `modal/modal.cpp:36-58` | `on_dismiss` | — |
| input_field | `input_field/input_field.cpp:30-52` | `on_submit` | — |
| data_table | `data_table/data_table.cpp:34-57` | `on_row_select` | `col_count` |
| list | `list/list.cpp:37-60` | `on_select` | `selected` |
| tabs | `tabs/tabs.cpp:37-60` | `on_change` | `active`, `count` |
| overlay | `overlay/overlay.cpp:50-75` | `on_hidden` | `overlay`, `timer`, `duration_ms` |

**Impact:** ~120 lines of pure duplication. Each copy is a maintenance liability.
**Risk:** LOW — all slot structs are trivially copyable POD.

---

### Finding 2: Disabled Setter Duplication (MEDIUM PRIORITY)

**Issue:** 7 widgets have identical `X_set_disabled(lv_obj_t*, bool)` bodies:

```cpp
void X_set_disabled(lv_obj_t *w, bool disabled) {
    if (w == nullptr) { return; }
    if (disabled) { lv_obj_add_state(w, LV_STATE_DISABLED); }
    else          { lv_obj_remove_state(w, LV_STATE_DISABLED); }
}
```

**Evidence:**
- `button/button.cpp:207-217`
- `slider/slider.cpp:148-158`
- `toggle/toggle.cpp:141-151`
- `knob/knob.cpp:130-140`
- `dropdown/dropdown.cpp:218-228`
- `list/list.cpp:265-275`
- `input_field/input_field.cpp:187-197`

**Impact:** ~35 lines of trivial duplication. Inconsistency risk if behavior needs to change.
**Risk:** LOW

---

### Finding 3: Data Pool Duplication and Inconsistency (LOW-MEDIUM PRIORITY)

**Issue:** Three display-only widgets (gauge, chart, vu_strip) have hand-rolled data pool logic. Gauge uses a non-standard split-array pattern; chart does extra initialization inside acquire; vu_strip has a minimal release that only clears `in_use`.

**3a. Gauge — split-array pool (gauge.cpp:62-87):**
```cpp
gauge_data g_gauge_data[kGaugeDataPoolSize] = {};
bool       g_gauge_data_used[kGaugeDataPoolSize] = {};

gauge_data *acquire_data() {
    for (int i = 0; i < kGaugeDataPoolSize; ++i) {
        if (!g_gauge_data_used[i]) { g_gauge_data_used[i] = true; return &g_gauge_data[i]; }
    }
    ...
}
void release_data(gauge_data *d) {
    for (int i = 0; i < kGaugeDataPoolSize; ++i) {
        if (&g_gauge_data[i] == d) { g_gauge_data_used[i] = false; return; }
    }
}
```

**3b. Chart — redundant init in acquire (chart.cpp:28-40):**
```cpp
chart_data *acquire_data() {
    for (auto &d : g_chart_data) {
        if (!d.in_use) {
            d.in_use = true;
            d.series_count = 0;                    // redundant if release zeroes
            for (auto &s : d.series) { s = nullptr; } // redundant if release zeroes
            return &d;
        }
    }
    ...
}
void release_data(chart_data *d) { if (d) d->in_use = false; }  // doesn't zero other fields
```

**3c. Vu_strip — standard pattern but hand-rolled (vu_strip.cpp:24-43):**
```cpp
vu_strip_meta *acquire_meta() {
    for (auto &m : g_vu_meta) {
        if (!m.in_use) { m.in_use = true; return &m; }
    }
    ...
}
void release_meta(vu_strip_meta *m) { if (m) m->in_use = false; }
```

**Impact:** ~45 lines of pool boilerplate across 3 files, inconsistent release behavior (gauge pointer-scan, chart no-zero, vu_strip flag-only).
**Risk:** LOW

---

### Findings Investigated but NOT Acted On

| Finding | Why left alone |
|---------|---------------|
| HTTP method translation duplication (`http_client.c` vs `http_server.c`) | Maps to different ESP-IDF types (`esp_http_client_method_t` vs `httpd_method_t`) — can't trivially share |
| Progress/level_bar `apply_bar_theme` similarity | Different color tokens used (`color_disabled` vs `color_outline_variant`, `color_primary` vs `color_accent`) — not actually duplicated |
| Script sdkconfig args duplication | Different argument shapes, ~8 lines saved, rarely touched — not worth cross-file dependency |
| HAL `src/common/` flat directory | 48 files but reorganization churns CMakeLists.txt for cosmetic gain only |
| Service callback-depth tracking | Only Bluetooth uses it; other services use simpler volatile-bool patterns that work fine for their simpler lifecycles |
| `on_lvgl_deleted` handler duplication | Each widget's handler casts to its own slot type in an anonymous namespace; extracting requires exposing slot types or template instantiation per widget — adds complexity for ~3 lines saved per widget |

### Positive Findings (No Action Needed)

- **No dead code detected** — all 48 HAL modules, 20 services, and 17 widgets are referenced
- **No over-engineered abstractions** — router/feedback/runtime layers are lean
- **Clean capability pattern** — simple open/handle/close with device/host split
- **Effective auto-discovery** — widget CMake GLOB prevents manual listing churn

---

## Phase 3 — Refactoring Batches

### Batch 1: Generalize `release_ui_slot` (Foundation)

**What:** Replace field-specific clearing with zero-initialization (`*slot = {}`) for all POD slot structs. This unblocks all subsequent widget migrations.

**File:** `components/blusys_framework/include/blusys/framework/ui/detail/fixed_slot_pool.hpp`

**Before (lines 3-7, 36-44):**
```cpp
// Slot types must be structs with members: in_use (bool), on_change (callback),
// user_data (void*). Do not use these helpers for ad-hoc layouts.
...
template <typename Slot>
void release_ui_slot(Slot *slot)
{
    if (slot != nullptr) {
        slot->on_change = nullptr;
        slot->user_data = nullptr;
        slot->in_use    = false;
    }
}
```

**After:**
```cpp
// Slot types must be POD structs with an `in_use` (bool) member.
// Zero-initialization clears all fields on release.
...
template <typename Slot>
void release_ui_slot(Slot *slot)
{
    if (slot != nullptr) {
        *slot = {};
    }
}
```

**Why safe:** All slot structs are trivially copyable POD. Zero-init sets: pointers → `nullptr`, bools → `false`, ints → `0`. This is identical to the "released" state for every field in every slot struct. Existing callers (slider, toggle) get identical behavior since `{nullptr, nullptr, false}` == `{}` for their 3-field structs.

**LOC delta:** -2
**Risk:** LOW
**Depends on:** Nothing

---

### Batch 2: Migrate 6 Simple Widgets to Template Helpers

**What:** Replace hand-rolled `acquire_slot()` and `release_slot()` with delegates to `detail::acquire_ui_slot` / `detail::release_ui_slot` in widgets with simple slot structs (no complex teardown).

**Per-widget change pattern:**
1. Add `#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"`
2. Replace `acquire_slot()` body:
   ```cpp
   button_slot *acquire_slot() {
       return detail::acquire_ui_slot(g_button_slots, kTag, "BLUSYS_UI_BUTTON_POOL_SIZE");
   }
   ```
3. Replace `release_slot()` body:
   ```cpp
   void release_slot(button_slot *slot) {
       detail::release_ui_slot(slot);
   }
   ```
4. Remove now-unused `#include <cassert>` and `#include "blusys/log.h"` where no other uses remain in the file

**Files (all under `components/blusys_framework/src/ui/widgets/`):**

| Widget | Acquire lines removed | Release lines removed | Notes |
|--------|----------------------|----------------------|-------|
| `button/button.cpp` | 37-49 (13 lines) | 52-59 (8 lines) | Keep `#include "blusys/log.h"` only if used elsewhere; button has no other BLUSYS_LOG calls so remove |
| `dropdown/dropdown.cpp` | 34-46 (13 lines) | 49-56 (8 lines) | Same pattern |
| `modal/modal.cpp` | 36-48 (13 lines) | 51-58 (8 lines) | Same pattern |
| `input_field/input_field.cpp` | 30-42 (13 lines) | 45-52 (8 lines) | Same pattern |
| `data_table/data_table.cpp` | 34-46 (13 lines) | 49-57 (9 lines) | Extra `col_count` zeros to 0, which is correct — re-set on create |
| `list/list.cpp` | 37-49 (13 lines) | 52-60 (9 lines) | Extra `selected` zeros to 0 (not -1), but list_create explicitly sets `slot->selected = -1` at line 194 |

**LOC delta:** ~-90
**Risk:** LOW — mechanical transformation, identical behavior
**Depends on:** Batch 1

---

### Batch 3: Migrate tabs and overlay to Template Helpers

**What:** Same migration for the two widgets with complex slot structs. Both `acquire_slot()` and `release_slot()` get migrated.

**tabs** (`tabs/tabs.cpp` lines 37-60):
- Extra fields `active` and `count` zero-init to `0`, which is correct — they're re-set on create
- Callback field is `on_change` (same as the original template assumed), but Batch 1 makes this irrelevant
- **Keep `#include "blusys/log.h"`** — tabs has a second `BLUSYS_LOGE` at line 151 for config validation
- Straightforward migration

**overlay** (`overlay/overlay.cpp` lines 50-75):
- Has complex slot: `on_hidden`, `user_data`, `overlay` (back-pointer), `timer`, `duration_ms`, `in_use`
- The `on_lvgl_deleted` handler (line ~122-128) calls `cancel_timer(slot)` BEFORE `release_slot(slot)` — timer cleanup happens before zero-init
- Zero-init sets `timer` to `nullptr` after timer is already deleted/cancelled — safe
- Migrate both acquire and release

**Files:**
- `components/blusys_framework/src/ui/widgets/tabs/tabs.cpp`
- `components/blusys_framework/src/ui/widgets/overlay/overlay.cpp`

**LOC delta:** ~-20
**Risk:** LOW-MEDIUM (overlay timer lifecycle needs attention but existing call order is correct)
**Depends on:** Batch 1

---

### Batch 4: Extract `set_disabled` Inline Helper (Independent)

**What:** Create a shared inline helper for the disabled-state pattern and migrate 7 widgets.

**New file:** `components/blusys_framework/include/blusys/framework/ui/detail/widget_common.hpp`
```cpp
#pragma once

#include "lvgl.h"

namespace blusys::ui::detail {

inline void set_widget_disabled(lv_obj_t *w, bool disabled)
{
    if (w == nullptr) {
        return;
    }
    if (disabled) {
        lv_obj_add_state(w, LV_STATE_DISABLED);
    } else {
        lv_obj_remove_state(w, LV_STATE_DISABLED);
    }
}

}  // namespace blusys::ui::detail
```

**Each widget's public function becomes a one-line delegate:**
```cpp
void button_set_disabled(lv_obj_t *button, bool disabled)
{
    detail::set_widget_disabled(button, disabled);
}
```

**Files modified (7, all under `components/blusys_framework/src/ui/widgets/`):**
- `button/button.cpp` (lines 207-217)
- `slider/slider.cpp` (lines 148-158)
- `toggle/toggle.cpp` (lines 141-151)
- `knob/knob.cpp` (lines 130-140)
- `dropdown/dropdown.cpp` (lines 218-228)
- `list/list.cpp` (lines 265-275)
- `input_field/input_field.cpp` (lines 187-197)

**LOC delta:** ~-25 (net, after new 18-line header)
**Risk:** LOW — public function signatures unchanged, inline helper is identical to replaced bodies
**Depends on:** Nothing (independent of Batches 1-3)

---

### Batch 5: Normalize Data Pool Widgets (gauge, chart, vu_strip)

**What:** Migrate all three display-only data-pool widgets to use `detail::acquire_ui_slot` / `detail::release_ui_slot`.

#### 5a. Gauge — normalize split-array to in_use field

**File:** `components/blusys_framework/src/ui/widgets/gauge/gauge.cpp`

**Changes:**
1. Add `bool in_use = false;` to `gauge_data` struct (after line 27)
2. Add standard pool size macro:
   ```cpp
   #ifndef BLUSYS_UI_GAUGE_POOL_SIZE
   #define BLUSYS_UI_GAUGE_POOL_SIZE 16
   #endif
   ```
3. Change array declaration from `gauge_data g_gauge_data[kGaugeDataPoolSize]` to `gauge_data g_gauge_data[BLUSYS_UI_GAUGE_POOL_SIZE]`
4. Remove `constexpr int kGaugeDataPoolSize = 16;` (line 63)
5. Remove `bool g_gauge_data_used[kGaugeDataPoolSize] = {};` (line 65)
6. Replace `acquire_data()` body → `return detail::acquire_ui_slot(g_gauge_data, kTag, "BLUSYS_UI_GAUGE_POOL_SIZE");`
7. Replace `release_data()` (lines 79-87, pointer scan) → `detail::release_ui_slot(d);`
8. Add `#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"`

**Why safe:** Zero-init sets `arc`, `label` to `nullptr`, `unit` to `nullptr`, `min`/`max` to `0`, `in_use` to `false`. All fields are explicitly set in `gauge_create` before use.

#### 5b. Chart — use template and remove redundant init in acquire

**File:** `components/blusys_framework/src/ui/widgets/chart/chart.cpp`

**Changes:**
1. Add `#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"`
2. Add pool size macro:
   ```cpp
   #ifndef BLUSYS_UI_CHART_POOL_SIZE
   #define BLUSYS_UI_CHART_POOL_SIZE 8
   #endif
   ```
3. Change array to use macro: `chart_data g_chart_data[BLUSYS_UI_CHART_POOL_SIZE]`
4. Remove `constexpr int kChartDataPoolSize = 8;` (line 25)
5. Replace `acquire_data()` (lines 28-40, including redundant series init) → `return detail::acquire_ui_slot(g_chart_data, kTag, "BLUSYS_UI_CHART_POOL_SIZE");`
6. Replace `release_data()` (lines 42-47) → `detail::release_ui_slot(d);`

**Why safe:** The current `acquire_data` zeros `series_count` and `series[]` inside the acquire loop. With `release_ui_slot`'s `*slot = {}`, these fields are already zeroed on release. The initial pool state (`= {}`) is also zeroed. So the explicit init was only necessary because the old `release_data` didn't zero on release — it just set `in_use = false`. After the migration, the zeroing in acquire becomes redundant.

#### 5c. Vu_strip — use template

**File:** `components/blusys_framework/src/ui/widgets/vu_strip/vu_strip.cpp`

**Changes:**
1. Add `#include "blusys/framework/ui/detail/fixed_slot_pool.hpp"`
2. Replace `acquire_meta()` body (lines 24-36) → `return detail::acquire_ui_slot(g_vu_meta, kTag, "BLUSYS_UI_VU_STRIP_POOL_SIZE");`
3. Replace `release_meta()` body (lines 38-43) → `detail::release_ui_slot(m);`

**Why safe:** All `vu_strip_meta` fields are re-set in `vu_strip_create` before use. Zero-init is correct released state.

**Total LOC delta for Batch 5:** ~-45
**Risk:** LOW
**Depends on:** Batch 1

---

## Phase 4 — Execution Order and Dependencies

```
Batch 1 (generalize release_ui_slot)
    │
    ├──→ Batch 2 (6 simple callback widgets)
    │        │
    │        └──→ Batch 3 (tabs + overlay)
    │
    └──→ Batch 5 (gauge + chart + vu_strip data pools)

Batch 4 (set_disabled helper) ──→ runs independently, any time
```

**Recommended execution order:** 1 → 2 → 3 → 5 → 4

Each batch is a single logical commit.

---

## Phase 5 — Verification

### Per-Batch Verification

After each batch:
1. `./blusys host-build` — host SDL2 build (fast compilation check)
2. `cd` to host build dir → `ctest` — spine tests + snapshot smoke
3. `./blusys lint` — layering enforcement
4. `python3 scripts/check-framework-ui-sources.py` — widget source list consistency

### Full Verification (After All Batches)

5. `./blusys build examples/reference/framework_ui_basic esp32s3` — device build exercising all widgets
6. `./blusys build examples/quickstart/interactive_controller esp32` — canonical interactive archetype
7. `./blusys build examples/quickstart/interactive_controller esp32c3` — second target
8. `./blusys build examples/quickstart/edge_node esp32` — headless archetype
9. `scripts/scaffold-smoke.sh` — scaffold + host CMake sanity

### Potential Regressions to Watch

| Area | What to check | Why |
|------|--------------|-----|
| Overlay auto-dismiss | Show overlay, let timer fire, verify hide callback | Timer lifecycle depends on slot release ordering |
| List selection state | Create list, select item, delete+recreate, verify no stale selection | `selected` field now zeros to 0 instead of being left uninitialized |
| Data table re-population | Set data, clear, set new data | `col_count` zeros to 0 on release |
| Gauge value display | Create gauge, update value, delete | Pool pattern fundamentally changed |
| Chart series lifecycle | Add series, push data, delete chart, create new | Redundant init removed from acquire; relies on release zeroing |
| Vu_strip re-creation | Create vu_strip, delete, create another | Release now zeros all fields vs. just `in_use` |

---

## Phase 6 — Summary

### Total Impact

| Metric | Value |
|--------|-------|
| Files modified | 14 `.cpp` + 1 `.hpp` modified |
| Files created | 1 `.hpp` (`detail/widget_common.hpp`) |
| Net LOC reduction | **~180 lines** |
| Public API changes | **None** |
| ABI changes | **None** |
| New abstractions | 1 inline helper function |
| Risk profile | All batches LOW or LOW-MEDIUM |

### What Improved
- **Consistency:** All 13 pool-based widgets now use the same template helpers (10 callback + 3 data pool)
- **Maintainability:** Pool behavior changes in one place, not 13
- **Clarity:** Gauge, chart, vu_strip use standard pool pattern matching callback widgets
- **DRY:** Disabled setter defined once, delegated everywhere

### Remaining Opportunities (Not in This Plan)

These are lower-priority items identified during the audit that could be addressed in future work:

1. **Widget `on_lvgl_deleted` handler duplication** — 10 widgets have identical 4-line handlers. Extracting requires exposing slot types or template instantiation per widget; adds complexity for ~30 total lines saved. Monitor if more widgets are added.

2. **Service callback lifecycle patterns** — Bluetooth's `callback_enter_locked`/`callback_exit_locked` (bluetooth.c:75-90) could be generalized for other connectivity services. Deferred because services tier stays C and each service's lifecycle is subtly different.

3. **Script `create.sh` inline templates** — Three `blusys_create_main_yml_*()` functions with ~95% overlap could use a single template with conditional blocks. Low risk but affects scaffolding.

4. **CMake optional component detection macro** — The two-name check pattern (`espressif__foo` vs `foo`) appears 6 times across 2 CMakeLists.txt files. A `blusys_find_optional_component()` macro would save ~4 lines per occurrence.

5. **Widget-author-guide.md update** — After all batches, update the guide to reference the generalized `acquire_ui_slot`/`release_ui_slot` as the standard and note that slot types no longer need a specific callback field name.

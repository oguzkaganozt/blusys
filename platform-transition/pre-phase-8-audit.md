# Pre-Phase-8 Audit Report

**Date:** 2026-04-09
**Branch:** `platform-transition` (7 commits ahead of `main`)
**Scope:** Full review of the changes that have landed since branching from `main`, performed before kicking off Phase 8.

## Overall verdict

The branch is in good shape. No compile-breaking bugs, no C++ discipline violations in the framework, no broken Phase 3 include-path migration, no `BLUSYS_PATH` leakage in scaffolded products. The findings below are all fixable without touching implementation — mostly doc drift accumulated across phases, plus one lint-script gap.

Nothing here blocks Phase 8. A short housekeeping commit would close every item listed.

## Branch summary

Seven commits on top of `main`, +7,877 / −606 lines across 126 files:

| # | Commit | Phase | What landed |
|---|---|---|---|
| 1 | `d1ae513` align repo with platform transition | 2 + 3 | Identity rename, drivers move into HAL, framework component stub, lint script |
| 2 | `7d85912` update transition guidance docs | 3 follow-up | CLAUDE.md / PROGRESS.md sync |
| 3 | `a2cc41b` add framework core and ui foundation | 4 + early 5/6 | C++ build policy, `blusys/log.h`, containers, router/intent/feedback/controller/runtime headers, theme + layout primitives |
| 4 | `43d40f0` land V1 widget kit and close Phase 6 | 5 + 6 | bu_button/toggle/slider/modal/overlay, encoder helpers, widget-author-guide, `framework_app_basic` end-to-end |
| 5 | `c715325` add Phase 4.5 host harness | 4.5 | `scripts/host/` + LVGL FetchContent + SDL2 + `blusys host-build` |
| 6 | `7d993d4` refresh canonical docs | docs sweep | README/CLAUDE/AGENTS/architecture/framework docs aligned to V1 surface |
| 7 | `3c9a642` close Phase 7 product scaffold | 7 | `blusys create --starter <headless\|interactive>`, four-CMakeLists scaffold, sample controllers/UI |

Phases 1–7 are closed. Phases 8 and 9 remain pending.

## Code audit

### `blusys_framework/` (~2,600 lines C++) — clean

- All 17 `.cpp` sources are registered in `components/blusys_framework/CMakeLists.txt`; headers pair with impls correctly.
- Five widgets follow the slot-pool / six-rule contract consistently. Button / toggle / slider default to pool size 32; modal / overlay default to 8 (intentional — rarer composition widgets).
- Every widget uses the semantic callback typedefs from `ui/callbacks.hpp` (`press_cb_t`, `toggle_cb_t`, `change_cb_t`, `release_cb_t`, `long_press_cb_t`); no widget redefines them locally.
- No `new` / `delete` / `std::vector` / `std::string` / `std::map` / exceptions / RTTI anywhere under `components/blusys_framework/src/`.
- No direct `ESP_LOG*` / `esp_log.h` usage in framework sources — logging all goes through `BLUSYS_LOG*` in `blusys/log.h`.
- Header and implementation namespaces match throughout (`blusys::framework::*`, `blusys::ui::*`).
- `examples/framework_app_basic/main/framework_app_basic_main.cpp` genuinely exercises the full spine chain: button `on_press` → `runtime.post_intent` → `runtime.step` → `controller.handle` → `slider_set_value` / `submit_route` → `ui_route_sink` → `overlay_show`, with feedback emitted at each step.

**One note, not a bug:** `components/blusys_framework/src/core/framework.cpp` is a stub — `init()` and `run_once()` are placeholder implementations. The real runtime logic lives in `runtime.cpp`, so this is fine in V1, but it is worth deciding whether `framework.cpp` should be deleted or filled in before the v6.0.0 tag.

### CLI scaffold (`blusys` script `cmd_create`) — clean

- Heredoc terminators are quoted correctly; no unintended `${...}` shell expansion at scaffold time.
- Top-level `CMakeLists.txt` template correctly uses `EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/app"` per the amended decision 15.
- `BLUSYS_BUILD_UI` flows end-to-end from `product_config.cmake` → top-level CMakeLists → `components/blusys_framework/CMakeLists.txt`. Headless scaffolds genuinely skip UI sources and LVGL.
- `main/idf_component.yml` conditionally adds `lvgl/lvgl ~9.2` only for interactive.
- Both reference outputs under `/tmp/blusys_phase7_test/` (`my_panel` interactive, `my_gateway` headless) match what the current script produces.

### Driver examples and include-path migration — clean

- All 9 `examples/<driver>_basic/main/CMakeLists.txt` files correctly declare `REQUIRES blusys` (not `blusys_services`) after the Phase 3 driver move.
- No stale pre-Phase-3 `blusys/<category>/<module>.h` include paths anywhere in the repo — the rewrites to `blusys/drivers/<category>/<module>.h` are complete.
- No `BLUSYS_PATH` leakage into scaffolded product templates. The only remaining `BLUSYS_PATH` references are in bundled monorepo examples under `examples/`, which is the correct scope.
- No dangling references to the deleted root-level `CPP-transition.md` or `UI-system.md` (both moved to `platform-transition/`).

## Doc inconsistencies (the real findings)

### 1. `blusys_all.h` sweep count is stale

`platform-transition/ROADMAP.md:415`, `CLAUDE.md:35`, `CLAUDE.md:47`, and `PROGRESS.md:91` all claim the sweep covers **15 files, 17 occurrences**. Ground truth from grep is **13 files, 15 occurrences**:

- **1 example:** `examples/ui_basic/main/ui_basic_main.c:3`
- **12 guides:**
  - `docs/guides/button-basic.md:19`
  - `docs/guides/console-basic.md:16`
  - `docs/guides/fatfs-basic.md:16`
  - `docs/guides/fs-basic.md:16`
  - `docs/guides/http-basic.md:17`
  - `docs/guides/http-server-basic.md:22`
  - `docs/guides/lcd-basic.md:15`, `:72`, `:125` (3 occurrences)
  - `docs/guides/mqtt-basic.md:21`
  - `docs/guides/ota-basic.md:31`
  - `docs/guides/ui-basic.md:16`
  - `docs/guides/wifi-prov-basic.md:18`
  - `docs/guides/ws-client-basic.md:22`

The ROADMAP expected 2 examples, but `examples/lcd_st7735_basic/main/lcd_st7735_basic_main.c` was already cleaned during Phase 3 (it now includes `blusys/blusys.h`). The CLI scaffold template was already cleaned in Phase 7. Neither cleanup was counted down in the ROADMAP.

Additionally, prose references that will need updating at the end of Phase 8 once the header is deleted:

- `docs/architecture.md:182` — "`blusys/blusys_all.h` still exists as a transitional compatibility header, but new code should include the specific tier headers instead."
- `docs/guides/framework.md:197` — mentions the `blusys_all.h` removal plan.
- `platform-transition/application-layer-architecture.md:132`, `:134` — describe the 17-occurrence sweep, also stale.
- `platform-transition/CPP-transition.md:75` — mentions the removal in Phase 8.
- `platform-transition/ROADMAP.md:139`, `:387`, `:433`, `:442`, `:507` — various mentions of the removal plan.

**Fix:** Update the counts in `ROADMAP.md:415`, `CLAUDE.md:35`, `CLAUDE.md:47`, `PROGRESS.md:91`, and `application-layer-architecture.md:132` to "13 files, 15 occurrences" and drop `examples/lcd_st7735_basic/main/lcd_st7735_basic_main.c` from the ROADMAP Phase 8 example list.

### 2. Decision 15 amendment has two stale spots in `ROADMAP.md`

The amendment (applied during Phase 7) says `EXTRA_COMPONENT_DIRS` points at `${CMAKE_CURRENT_LIST_DIR}/app`, not at the project root. `CLAUDE.md`, `PROGRESS.md`, the scaffolded template in the `blusys` script, the actual template at `ROADMAP.md:339`, and `DECISIONS.md` are all correct. Two spots in `ROADMAP.md` still contradict the amendment:

- **`ROADMAP.md:42`** (Target end-state file diagram) — `EXTRA_COMPONENT_DIRS="."` should reflect the `/app` scoping.
- **`ROADMAP.md:316`** (Phase 7 prose) — "The top-level `CMakeLists.txt` sets `EXTRA_COMPONENT_DIRS` to the project root so ESP-IDF discovers `app/` during its component scan." This is wrong; the correct wording is in the template immediately below at line 339.

**Fix:** Update `ROADMAP.md:42` and `ROADMAP.md:316` to match the amendment. One-liner prose change; no code touched.

### 3. `CLAUDE.md:31` pool-size statement is imprecise

> "Each Camp 2 widget uses a fixed-capacity slot pool keyed by `BLUSYS_UI_<NAME>_POOL_SIZE` (default 32)"

Modal and overlay default to 8, not 32. The `components/blusys_framework/widget-author-guide.md` documents the per-widget defaults correctly. Reword CLAUDE.md to match — e.g. "default 32 for button/toggle/slider, 8 for modal/overlay".

### 4. `docs/architecture.md:182` will need a Phase 8 touch

Currently accurate ("`blusys/blusys_all.h` still exists as a transitional compatibility header…"), but the Phase 8 sweep must update or remove this line, along with `docs/guides/framework.md:197`, once the header is deleted. Not a bug today — add to the Phase 8 checklist so it is not missed.

## Infrastructure gap

### 5. `scripts/lint-layering.sh` has holes

The script is wired into the `blusys lint` subcommand but **not** into `.github/workflows/ci.yml`, so it only runs when a developer remembers to run it locally. Additional smaller issues:

- Only matches double-quoted `#include "blusys/drivers/..."`. Angle-bracket `#include <blusys/drivers/...>` would slip past. Unlikely in practice but trivially closable.
- The driver allowlist is `blusys_lock.h`, `blusys_esp_err.h`, `blusys_timeout.h` (per Phase 3 spec). The script silently rejects `blusys_target_caps.h`, which drivers might legitimately need for capability checks. Worth deciding the policy now rather than waiting for the first driver that wants it.

**Fix options:** (a) extend the regex to cover `<...>`; (b) add a `blusys lint` step to `.github/workflows/ci.yml` so the check gates PRs; (c) decide whether `blusys_target_caps.h` should be on the driver allowlist.

## Phase 8 scope nuance (not a bug)

Phase 8's done-when criteria in `ROADMAP.md` require at least 3 framework examples covering:

1. A screen with layout primitives only (labels + rows + cols).
2. A screen with interactive widgets (button + slider + toggle).
3. An encoder-driven focus-traversal example using the input helpers.

The three landed examples are:

- **`framework_core_basic`** — spine only, no LVGL. Does not match any of the three scenarios.
- **`framework_ui_basic`** — theme + primitives + interactive widgets mixed. Covers (1) and (2) partially but not cleanly separated.
- **`framework_app_basic`** — full spine + widgets + simulated button-driven input. Covers (2), and simulates (3) with buttons rather than a real `lv_indev_t` encoder.

None is a pure layout-only example, and the encoder traversal example uses simulated button input rather than encoder hardware. Two options before Phase 8 kicks off:

- **(a)** Accept that `framework_core_basic` + `framework_ui_basic` + `framework_app_basic` is the right scope and rewrite the Phase 8 done-when criteria to describe them accurately.
- **(b)** Add a fourth example (pure layout-only, or real `lv_indev_t` encoder hook) to strictly meet the current criteria.

Decide before starting Phase 8 so the criteria and the example set agree.

## Suggested pre-Phase-8 housekeeping

A single small commit before Phase 8 proper can close every documentation finding:

1. Fix `ROADMAP.md:42` and `ROADMAP.md:316` (decision 15 prose drift).
2. Update the `blusys_all.h` sweep count to "13 files, 15 occurrences" in `ROADMAP.md:415`, `CLAUDE.md:35`, `CLAUDE.md:47`, `PROGRESS.md:91`, and `application-layer-architecture.md:132`.
3. Drop `examples/lcd_st7735_basic/main/lcd_st7735_basic_main.c` from the Phase 8 sweep list in `ROADMAP.md`.
4. Fix `CLAUDE.md:31` pool-size wording to reflect the modal/overlay defaults.
5. Decide on the framework examples question (rewrite done-when or add a fourth) and update `ROADMAP.md` accordingly.
6. Optional: wire `blusys lint` into `.github/workflows/ci.yml`, extend its regex to `<...>` includes, and decide on `blusys_target_caps.h` accessibility for drivers.

None of this is blocking. The code itself is sound.

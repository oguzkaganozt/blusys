# Roadmap: product app readability (`blusys_framework`)

This document is the **forward plan** for keeping typical product code short, obvious, and idiomatic C++ on top of the reducer model. Product requirements stay in [`PRD.md`](PRD.md); contributor rules in [`CLAUDE.md`](CLAUDE.md).

---

## Where we are (foundation shipped)

The following is **already in the tree** and is the baseline for all future work:

- **`app_ctx::product_state<T>()`** — screen factories and bridges read app state through `ctx`, not file-static pointers.
- **Enum-friendly navigation** — `navigate_to` / `navigate_push` / `show_overlay` / `hide_overlay` accept `enum class` route/overlay ids where products define them.
- **Integration helpers** — [`integration_events.hpp`](components/blusys_framework/include/blusys/app/integration_events.hpp), [`reference_build_profile.hpp`](components/blusys_framework/include/blusys/app/reference_build_profile.hpp) reduce duplicated `#ifdef` matrices for the interactive reference shape.
- **View facade** — [`view/bindings.hpp`](components/blusys_framework/include/blusys/app/view/bindings.hpp) extended with gauge / level bar / VU setters and **`sync_percent_output`** (composite).
- **Navigation** — [`screen_router::register_screens`](components/blusys_framework/include/blusys/app/view/screen_router.hpp) for static tables; tab vs push called out in header comments.
- **Settings** — stable **`setting_item::id`**; callbacks receive **`setting_id`** (or row index when `id == 0`).
- **Canonical example** — [`examples/quickstart/interactive_controller`](examples/quickstart/interactive_controller): **`ShellChrome` / `HomePanel` / `StatusPanel`**, strong **`route`** / **`overlay`** enums, no global `g_state`.
- **Guidelines** — [Product application C++ shape](docs/internals/guidelines.md) in `docs/internals/guidelines.md`.

**Not every example or archetype has been migrated** to that shape yet; the roadmap below closes that gap and pushes the framework API further.

---

## Goals for the next phase

1. **One obvious pattern everywhere** — quickstarts, reference examples, and scaffold **read the same** (state via `ctx`, named panels, strong ids), not only the canonical controller.
2. **More vocabulary in the framework** — small **types and helpers** that remove repetition without hiding control flow (no reactive engine, no tier collapse).
3. **C++ used deliberately** — aggregates, `enum class`, optional `variant` for actions where teams want exhaustiveness; **not** deep LVGL-owning class hierarchies.

---

## Wave A — Rollout and consistency

**Problem:** Reference apps (`interactive_panel`, `gateway`, others) still mix **legacy `g_state`**, manual loops, or old settings assumptions with the new APIs.

**Outcomes**

- Migrate **remaining interactive examples** to **`product_state`**, **`register_screens`** where applicable, and **settings `id`** + new callback signature consistently.
- **Scaffold template** (`templates/scaffold/...`) updated so new products **start** in the modern shape.
- **Host scripts** (`scripts/host/src/*.cpp`) aligned if they still demonstrate the old spelling.

**Proof:** `grep -r g_state examples/` only hits intentional escape hatches (or zero hits); PR CI quickstarts build unchanged in behavior.

---

## Wave B — Framework vocabulary (types, not boilerplate)

**Problem:** Products still repeat **`static_cast<std::uint32_t>(route::foo)`** in tables; `reference_build` names are **interactive-centric**; capability **`map_event`** in large products remains switch-heavy even with helpers.

**Outcomes**

1. **Route table API** — overload or helper that accepts **`enum class` route** in `screen_registration` rows (store `std::uint32_t` internally; conversion at one boundary).
2. **Neutral naming** — split or generalize **`reference_build_profile`** into **`blusys::app::build_profile`** (or similar): *device profile + labels + host window* helpers that are not named “interactive_*” so edge / panel / gateway reuse the same entry style.
3. **Capability → action** — extend **stock helpers** beyond storage/provisioning (e.g. connectivity / telemetry / OTA “sync snapshot” one-liners) **or** a **small macro/table DSL** that expands to explicit `map_event` bodies (still grep-friendly).
4. **Optional `std::variant` action model** — documented **opt-in** path for new apps: `using Action = std::variant<...>;` with `update` dispatch via `std::visit` — **not** required for existing apps.

**Proof:** At least one **non-controller** example uses the generalized profile helper without copy-paste `#ifdef` blocks; `map_event` in that example stays under a **line budget** agreed in review.

---

## Wave C — View layer: composites and panel toolkit

**Problem:** Only **one** composite (`sync_percent_output`) exists; other screens still hand-roll multiple `view::set_*` calls; **route-scoped handles** are a **named pattern** in docs but not a **tiny shared type** in headers.

**Outcomes**

1. **2–4 named composites** in `view::bindings` (or `view/composites.hpp`) driven by **real** examples — e.g. status row (badge + two KV lines), simple metric strip — **null-safe**, fixed surface area.
2. **Optional `panel_handle_group` struct** (or similar) in the framework: holds a **fixed-capacity** or explicit set of `lv_obj_t*` with **`clear()`** only — **no** ownership / `lv_obj_delete` from app code; document alignment with `screen_lifecycle.on_hide`.
3. **Prefer `view::set_text` everywhere** in examples for labels — reserve raw `lv_label_set_text` for generated code or escapes.

**Proof:** `interactive_controller` **`sync_*`** stays thin; a **second** archetype example adopts one new composite without forking LVGL.

---

## Wave D — Docs, diagram, optional tooling

**Outcomes**

- **One diagram** in docs: **integration event / intent → `map_*` → action queue → `update` → `view::` sync** — linked from [`guidelines.md`](docs/internals/guidelines.md) and/or app docs.
- **Optional codegen** (routes, action tags, settings ids): **only** if measured **line-count and churn** win on `interactive_controller` + one other app; **never** mandatory for CI.
- **Optional** `app_identity` / shell **presets per archetype** — thin structs that fill theme + shell density hints.

**Proof:** New hire can answer “where does this event become UI?” using the diagram + one example file.

---

## Non-negotiables (unchanged)

- **Reducer center:** `update(ctx, state, action)` remains the single locus of domain rules.
- **Three tiers** — no collapse; HAL/services stay C where they are today.
- **No mandatory codegen**; **no** large reactive binding layer.
- **LVGL ownership** — registry/shell own teardown; app **`clear()`** only nulls handles unless explicitly documented otherwise.
- **Capabilities** remain the product-facing word — not “bundles.”

---

## Success criteria (next phase “done”)

| Criterion | Measure |
|-----------|--------|
| Pattern parity | All **PR CI quickstarts** + agreed reference examples use **`product_state`** + strong ids; no stray **`g_state`**. |
| Less integration noise | New products’ **`integration/app_main.cpp`** dominated by **spec + capabilities + entry macro**, not preprocessor matrices. |
| Thinner sync | Core **`sync_*`** functions mostly call **composites** + panel methods; raw `blusys::ui::*` from app code is rare and justified. |
| Discoverability | Diagram + guidelines answer routing, actions, and sync without reading **`app_runtime.hpp`**. |

---

## Risks

| Risk | Mitigation |
|------|------------|
| Composites become a mini template language | Cap count and parameters; reject generic “bind anything” APIs |
| `variant` actions split the ecosystem | Strictly opt-in; document migration from tag+struct |
| Generalized `build_profile` overfits one product | Name APIs by **concern** (device vs host window), not by archetype |
| Codegen maintenance | Ship only if two apps share the generator; keep hand-written path first-class |

---

## Tracking (high level)

| Wave | Focus |
|------|--------|
| A | Example + scaffold + host parity rollout |
| B | Typed route tables, neutral build helpers, richer `map_event` helpers, optional `variant` actions |
| C | Composites + optional panel handle group type; second archetype proof |
| D | Diagram + optional codegen / presets |

---

## Explicit non-goals for this document

- Replacing LVGL with a custom retained tree.
- Owning LVGL objects in **`std::unique_ptr`** by default in product code.
- Merging **`blusys_framework`** with lower tiers.

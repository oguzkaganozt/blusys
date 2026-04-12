# Golden path and escape hatches

The **recommended product path** is **`blusys::app`**: `app_spec`, `update(ctx, state, action)`, `main/core` · `main/ui` · `main/integration/`, capabilities for connected behavior, and UI changes through **actions** and **`blusys::app::view::`** helpers—not raw LVGL or ad hoc service wiring.

## Top anti-patterns (avoid by default)

1. **Raw LVGL in product screens** outside custom widgets or an explicit custom view scope—bypasses routing/locks and makes behavior hard to reason about. Prefer **actions** and approved view helpers ([App runtime model](app-runtime-model.md)).
2. **Direct Wi‑Fi / connectivity orchestration** in `main/ui` or scattered across product files—use **capabilities** and thin **`integration/`** assembly so the framework can own lifecycle ([Capability composition](capability-composition.md)).
3. **Large domain logic in `integration/`**—that folder should stay **wiring** (profile, capability list, `map_event` / bridges). Heavy state machines belong in **`core/`** (reducer) or, for recurring connected flows, in **framework-level flows** as they land.

## UI layout

Prefer **`view::row`**, **`view::col`**, and **`page_create_in`** inside the shell so LVGL flex and scroll stay consistent. Stock display widgets should size from parent constraints (see `bu_gauge`). Details: [UI layout and LVGL flex](../internals/ui-layout-lvgl.md).

## When to use escape hatches

HAL and `blusys_services` are **supported** for advanced cases; use them when the canonical path cannot yet express what you need, and isolate that code behind clear boundaries. See [Architecture — tier model](../internals/architecture.md) and [Guidelines](../internals/guidelines.md).

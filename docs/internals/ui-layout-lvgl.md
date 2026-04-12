# UI layout and LVGL flex (framework)

This note describes how **Blusys framework** uses **LVGL’s built-in flex and scroll** so product code can declare structure (columns, rows, shell) without reimplementing layout rules in every app.

## Principles

1. **Prefer LVGL flex** (`LV_LAYOUT_FLEX`, `lv_obj_set_flex_flow`, `lv_obj_set_flex_align`) for composition. Do not add a parallel layout engine on top unless there is a strong reason.
2. **Bound the scroll viewport** — In the interaction shell, the scrollable page column must be a **flex child with a bounded height** (see `shell` `content_area` + `page_create_in`). Otherwise the scroll area can grow with content and push chrome (e.g. tab bar) off-screen.
3. **Stock widgets size to constraints** — Widgets placed in flex rows with a fixed row height must respect the **allocated** size (e.g. `bu_gauge` scales its arc on `LV_EVENT_SIZE_CHANGED`, using `blusys::ui::flex_layout::effective_cross_extent_for_row_child` when a flex row’s inner height is larger than the child’s content height).
4. **Escape hatch** — Custom product UI may use raw LVGL inside **custom widgets** or an explicit custom scope; keep the same contracts (focus, actions, no direct runtime bypass) as in `widget-author-guide.md`.

## Primitives

- **`col` / `row`** — Flex column/row helpers. `row_config` / `col_config` expose **main / cross / track** flex align (`row` defaults: `START`, `CENTER`, `CENTER`; `col` defaults: `START`, `START`, `START`) so strips and stacks can tune alignment without raw `lv_obj_set_flex_align` at every call site.
- **`blusys::ui::flex_layout`** — Helpers for flex **row** / **column** parents (`effective_cross_extent_for_row_child`, `effective_cross_extent_for_column_child`) when a child’s content-sized dimension under-reports the strip.
- **`page_create` / `page_create_in`** — Page column; in-shell variant sets `flex_grow(1)`, `min_height(0)`, and optional scroll so the column fills `content_area` correctly.

## Shell

The shell root is a **column flex**: header, optional status, **content_area** (`flex_grow(1)`), optional tab bar. `content_area` is itself a **column flex** container so its single child (the scrollable page column) participates as a proper flex item.

## See also

- [Architecture](architecture.md) — tier model  
- [Guidelines](guidelines.md) — API and product layout rules  
- `components/blusys_framework/widget-author-guide.md` — widget contract  

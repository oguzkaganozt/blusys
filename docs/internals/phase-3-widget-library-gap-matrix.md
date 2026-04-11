# Phase 3 — Widget library gap matrix

This note maps **Phase 3** in `ROADMAP.md` (repository root) to the codebase, with a short contract/focus note. It is a living checklist for the shared widget expansion program.

## Legend

| Status | Meaning |
|--------|---------|
| shipped | Implemented and used in the stock kit |
| partial | Present but may need more polish or docs emphasis |
| n/a | Covered elsewhere (e.g. primitive vs interactive folder) |

## Shared core widgets

| Roadmap | Location | Status | Notes |
|---------|----------|--------|--------|
| `progress` | `framework/ui/widgets/progress/` | shipped | Display-only; theme track + primary indicator |
| `list` | `framework/ui/widgets/list/` | shipped | Interactive; slot pool; selection callback |
| `card` | `framework/ui/widgets/card/` | shipped | Layout container with optional title |
| `icon_label` | `framework/ui/primitives/icon_label` | shipped | Primitive (no callbacks) |
| `status_badge` | `framework/ui/primitives/status_badge` | shipped | Primitive; semantic badge levels |
| `input_field` | `framework/ui/widgets/input_field/` | shipped | Interactive; keyboard/textarea bridge |
| `tabs` | `framework/ui/widgets/tabs/` | shipped | Interactive; slot pool |
| `dropdown` | `framework/ui/widgets/dropdown/` | shipped | Interactive; options joined in static buffer |

## Consumer emphasis

| Roadmap | Location | Status | Notes |
|---------|----------|--------|--------|
| `gauge` | `framework/ui/widgets/gauge/` | shipped | Arc readout |
| `knob` | `framework/ui/widgets/knob/` | shipped | Rotary control |
| Audio-friendly displays | `framework/ui/widgets/level_bar/`, `framework/ui/widgets/vu_strip/` | shipped | Thin level bar + segmented VU strip |

## Industrial emphasis

| Roadmap | Location | Status | Notes |
|---------|----------|--------|--------|
| `data_table` | `framework/ui/widgets/data_table/` | shipped | Scrollable rows |
| `chart` | `framework/ui/widgets/chart/` | shipped | Series pool; LVGL chart |
| `key_value` | `framework/ui/primitives/key_value` | shipped | Primitive |

## Umbrella headers

| Header | Role |
|--------|------|
| `blusys/framework/ui/primitives.hpp` | All layout primitives |
| `blusys/framework/ui/widgets.hpp` | Full interactive + display stock widget set |

## Contract checklist (six-rule + focus)

For each **interactive** widget: theme tokens only; `<name>_config`; semantic callbacks from `callbacks.hpp`; slot pool where required; `LV_EVENT_DELETE` releases slots; setters own transitions; focus/disabled styles aligned with `theme_tokens`.

For **display-only** widgets (`progress`, `chart`, `level_bar`, `vu_strip`, primitives): rules 1, 2, 5, 6 of the widget author guide (`components/blusys_framework/widget-author-guide.md` in the repository); no raw LVGL in product code. For architecture context see [Architecture](architecture.md).

## Validation

- `blusys lint`
- `blusys host-build <quickstart>` for LVGL-on-host coverage
- `blusys build` per `inventory.yml` target matrix for touched examples

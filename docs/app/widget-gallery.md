# Widget Gallery

Common stock widgets in `blusys::app` and where they fit. Visuals should come from theme tokens (see [App](index.md)) only; keep raw LVGL inside custom widgets.

For layout across display sizes, see [Profiles](profiles.md). For how screens tie together, see [Views & Widgets](views-and-widgets.md).

## Compact Control

Best for **handheld** / compact interactive surfaces and dense control strips.

| Widget | Role | Often appears in |
|--------|------|------------------|
| `button` | Primary actions, confirmations | Settings rows, modals, shell actions |
| `toggle` | On/off and binary settings | Settings, feature flags |
| `slider` | Ranged numeric control | Brightness, thresholds |
| `gauge` | Compact numeric feedback | Status strips, monitoring |

Reference starter: `examples/quickstart/handheld_starter/` (host + ST7735 path; see [Product shape](../start/product-shape.md)).

## Operational Surface

Best for **surface** interactive and gateway local UI: dashboards and multi-section screens.

| Widget | Role | Often appears in |
|--------|------|------------------|
| `card` | Grouped content and summaries | Dashboard grids |
| `list` | Options and drill-down | Settings, device lists |
| `tabs` | Screen partitioning | Multi-section flows |
| `status_badge` | Short operational state | Shell chrome, headers |

Reference starter: `examples/reference/surface_ops_panel/`.

## Data And Diagnostics

Shared by surface interactive and connected products when showing health and metrics.

| Widget | Role | Often appears in |
|--------|------|------------------|
| `key_value` | Paired status fields | Status and about screens |
| `data_table` | Tabular operational rows | Diagnostics, logs |
| `progress` | Long-running work | OTA, loading flows |
| `dropdown` | Compact single choice | Filters, mode picks |

Reference starters: `examples/quickstart/headless_telemetry/`, `examples/reference/surface_gateway/`.

## Notes

- Keep visuals driven by theme tokens.
- Use custom widgets only when the stock set no longer fits.
- Keep raw LVGL inside the widget implementation, not in app code.

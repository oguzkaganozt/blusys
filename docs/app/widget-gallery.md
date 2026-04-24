# Widget Gallery

Pick widgets by the kind of screen you are building.

## Compact control

Best for dense interactive devices.

| Widget | Use it for |
|--------|------------|
| `button` | primary actions |
| `toggle` | binary settings |
| `slider` | ranged values |
| `gauge` | compact feedback |

## Operational surface

Best for dashboards and multi-section local UIs.

| Widget | Use it for |
|--------|------------|
| `card` | grouped summaries |
| `list` | options and drill-down |
| `tabs` | screen partitioning |
| `status_badge` | short operational state |

## Data and diagnostics

Best for health screens and long-running work.

| Widget | Use it for |
|--------|------------|
| `key_value` | paired status fields |
| `data_table` | diagnostics and logs |
| `progress` | OTA and loading states |
| `dropdown` | compact single choice |

## Notes

- Drive visuals from theme tokens.
- Keep raw LVGL inside custom widgets.
- Use the stock set first; build custom widgets only when needed.

## Next steps

- [Views & widgets](views-and-widgets.md)
- [Profiles](profiles.md)
- [Interactive Quickstart](../start/quickstart-interactive.md)

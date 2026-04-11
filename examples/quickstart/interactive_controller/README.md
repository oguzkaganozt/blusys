# Interactive controller (primary archetype reference)

Forkable **interactive controller** reference: compact, tactile, encoder-first control on the shared `blusys::app` model.

## Locked brief

| Area | Choice |
|------|--------|
| **Purpose** | Single primary control loop (“Pulse Engine”) with accents, output level, and hold — feels like a product, not a widget gallery. |
| **Theme** | `expressive_dark` |
| **Inputs** | Encoder primary (intents → level delta, confirm, hold toggle); host SDL mirrors with keyboard focus. Buttons supplement; touch optional via framework bridges. |
| **Core screens** | Home (control), Status (snapshot + empty state → setup), Settings (accent, hold, about), Setup (provisioning flow), About |
| **Capabilities** | Storage (SPIFFS), provisioning (BLE bootstrap); connectivity optional for this reference |
| **Productization bar** | Strong motion/feedback on level and hold; shell badge reflects setup readiness; no raw LVGL outside `ui/` composition via `blusys::app::view` and stock widgets |

## Layout

- `core/` — `state`, `action`, `update()`, accent names, intent mapping
- `ui/` — screen factories registered on `screen_router`, shell tabs
- `integration/` — `app_spec`, ST7735/ST7789/host profile, capabilities, `map_event`

## Run

From the **Blusys repo root** (so the `blusys` CLI sets `BLUSYS_PATH`):

```bash
blusys host-build examples/quickstart/interactive_controller
# → examples/quickstart/interactive_controller/build-host/interactive_controller_host
blusys build examples/quickstart/interactive_controller esp32
```

See [Interactive Quickstart](../../../docs/start/quickstart-interactive.md) for the full host-first flow.

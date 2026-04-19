# Interactive controller (handheld quickstart)

Forkable **handheld** interactive reference: compact, tactile, encoder-first control on the shared `blusys::app` model.

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
- `platform/` — `app_spec`, ST7735/ST7789/host profile, capabilities, `on_event`

## Run

From the **Blusys repo root** (so the `blusys` CLI sets `BLUSYS_PATH`):

```bash
blusys host-build examples/quickstart/handheld
# → examples/quickstart/handheld/build-host/handheld_host
blusys build examples/quickstart/handheld esp32
```

## What you see on host

The host binary opens an SDL window showing the Home screen ("Pulse Engine" control). Keyboard maps to encoder intents: **arrow keys** step the output level, **Enter** confirms / toggles hold, **Tab** cycles focus. The shell badge changes as the Status screen's setup flow completes.

## What to edit first

When forking into a new product:

1. `core/app_logic.{hpp,cpp}` — replace the state/action model and `update()` body; keep the reducer pattern.
2. `ui/app_ui.{hpp,cpp}` and `ui/panels.{hpp,cpp}` — rename/restyle screens; keep routing through `screen_router` and dispatch via actions.
3. `platform/app_main.cpp` — swap profile, adjust the `capability_list`, and update `on_event` for any new capability events.

When adding a new capability, see [Authoring a capability](../../../docs/app/capability-authoring.md).

See [Interactive Quickstart](../../../docs/start/quickstart-interactive.md) for the full host-first flow.

# Product shape (interface, capabilities, policy)

New Blusys products are described by three explicit axes — not by named “starter bundles.” The scaffold and checked-in **`blusys.project.yml`** use the same vocabulary.

## Grammar

```bash
blusys create [--interface handheld|surface|headless] [--with cap1,cap2,...] [--policy policy1,...] [path]
blusys create --list
```

- **`--interface`** (single): how the product presents itself locally — shell, theme bias, UI on/off, host build defaults.
- **`--with`** (multi): [framework capabilities](../app/capabilities.md) — connectivity, storage, telemetry, OTA, and the rest (see `--list`).
- **`--policy`** (multi): cross-cutting behavior that is **not** a runtime capability — e.g. `low_power`.

Defaults: **`handheld`**, empty capabilities, empty policies. Interactive onboarding still runs `blusys create` with no arguments and shows the equivalent canonical command before generation.

## Interfaces

| Interface | Role | Typical use |
|-----------|------|-------------|
| **handheld** | Compact interactive shell, tactile bias, encoder-friendly | Small displays, control loops, settings |
| **surface** | Denser operator shell, dashboard-style | Larger panels, status at a glance |
| **headless** | No local UI by default; headless entry | Telemetry, gateways, invisible-when-healthy devices |

All use the same **`core/`** · **`ui/`** (if interactive) · **`integration/`** layout and **`update(ctx, state, action)`**.

## Reference examples in this repo

Examples keep the fixed `main/` layout; **`inventory.yml`** records each example’s `interface`, `capabilities`, and `policies` for CI and docs.

| Direction | Example path | Notes |
|-----------|--------------|--------|
| Handheld interactive | `examples/quickstart/handheld_starter/` | PR quickstart; compact ST7735-class path |
| Surface interactive | `examples/reference/surface_ops_panel/` | Operational / dashboard density (larger display profiles) |
| Headless connected | `examples/quickstart/headless_telemetry/` | Connected stack without a local UI by default |
| Surface + connected | `examples/reference/surface_gateway/` | Coordinator / operator device; optional local UI |
| Headless LAN control | `examples/reference/headless_lan_control/` | Minimal connectivity + `lan_control` (reference; see also `validation/lan_control_basic`) |
| Handheld Bluetooth | `examples/reference/handheld_bluetooth/` | `storage` + `bluetooth` on the interactive ST7735 path |

For historical minimal connectivity demos, see `examples/reference/connected_device/` and `examples/reference/connected_headless/`.

## Capabilities and policies

- **Capabilities** are composed in **`integration/`**; the reducer receives work via **actions**; capability events map to actions in **`integration/`** (see [Capability composition](../app/capability-composition.md)).
- **Policies** adjust defaults and integration overlays (e.g. power-related `sdkconfig` fragments) without adding a new capability kind.

Dependency and target rules (e.g. `telemetry` requires `connectivity`, `usb` targets ESP32-S3) are enforced by the generator and listed under **`blusys create --list`**.

## Build layout for generated projects

`blusys create` writes a top-level `CMakeLists.txt` that sets `EXTRA_COMPONENT_DIRS`
to the Blusys `components/` tree from the checkout used at generation time (embedded
path). Managed dependencies (ESP-IDF component registry, LVGL, etc.) stay in
`main/idf_component.yml`. If you move the project or build outside the original tree,
update `EXTRA_COMPONENT_DIRS` or vendor the platform components — see
[Architecture](../internals/architecture.md) (consumption models).

## Rules

- No raw LVGL in normal product screens — use `blusys::app::view`, stock widgets, and the custom widget contract.
- Keep **`integration/`** thin: profiles, capability list, `map_event`, bridges; heavy logic stays in **`core/`**.

## Next pages

- [Interactive quickstart](quickstart-interactive.md)
- [Headless quickstart](quickstart-headless.md)
- [App](../app/index.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)

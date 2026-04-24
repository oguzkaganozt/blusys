# Product shape

`blusys.project.yml` captures the shape of a product: `schema`, `interface`, `capabilities`, `profile`, and `policies`.

!!! note
    `blusys create --list` shows the live catalog. In a terminal, `blusys create` also offers the same choices in a small prompt.

## Shape fields

| Field | What it controls | Default |
|-------|------------------|---------|
| `schema` | manifest version | `1` |
| `interface` | local UI on or off | `interactive` |
| `capabilities` | framework modules to wire | `[]` |
| `profile` | interactive hardware preset or `null` | `null` |
| `policies` | non-capability overlays | `[]` |

## Grammar

```bash
blusys create [--interface interactive|headless] [--profile profile] [--with cap1,cap2,...] [--policy policy1,...] [path]
blusys create --list
```

- **`--interface`** chooses the runtime shape.
- **`--with`** adds framework capabilities such as connectivity, storage, telemetry, OTA, diagnostics, or LAN control.
- **`--profile`** selects an interactive device profile; leave it `null` for the interface default.
- **`--policy`** applies an overlay such as `low_power`.

## Good starting shapes

| Goal | Command |
|------|---------|
| blank interactive | `blusys create my_product` |
| compact display | `blusys create --interface interactive --profile st7735_160x128 --with connectivity,storage my_panel` |
| connected headless | `blusys create --interface headless --with connectivity,telemetry,ota --policy low_power my_sensor` |

!!! note
    Profile catalogs are interactive-only today. Headless products use `BLUSYS_APP_HEADLESS` without a display profile.

!!! warning
    Capability rules are validated. For example, `telemetry` requires `connectivity`, and `ota` needs connectivity, Bluetooth, or USB.

## What this means

- **Capabilities** are wired in product code and turned into actions by the reducer path.
- **Profiles** are named hardware presets selected by the manifest.
- **Policies** adjust defaults and overlays without introducing another capability type.

## Next steps

- [Interactive quickstart](quickstart-interactive.md)
- [Headless quickstart](quickstart-headless.md)
- [App](../app/index.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)

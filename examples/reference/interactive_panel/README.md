# Interactive panel (secondary archetype reference)

**Interactive panel** reference: denser, operational dashboard on the same reducer, shell, and capability stack as the controller — proves reuse without a second architecture.

## Locked brief

| Area | Choice |
|------|--------|
| **Purpose** | Local operator view: load trend, queue/heap/storage table, and unified status (connectivity + diagnostics + storage visibility). |
| **Theme** | `operational_light` |
| **Inputs** | Touch / button row / encoder per framework bridges; encoder intents map to mode and navigation shortcuts on host. |
| **Core screens** | Dashboard (chart + metrics table), Status (stock status screen with connectivity + diagnostics + storage), Settings (mode + about), About |
| **Capabilities** | Connectivity (Wi‑Fi + SNTP + mDNS on device; simulated on host), diagnostics snapshots, storage (SPIFFS) |
| **Productization bar** | Information-dense but readable; status surface matches industrial “glanceable ops” bias; intentionally less custom chrome than the controller |

## Display targets

Device builds default to **ILI9341** or **ILI9488** (see `main/Kconfig.projbuild`) for medium/large panels; host SDL matches via `BLUSYS_IP_HOST_DISPLAY_PROFILE`. This is separate from the canonical ST7735 **controller** path in Phase 5 validation.

## Layout

- `core/` — sampled load history, mode, diagnostics/storage sync actions
- `ui/` — dashboard composition, stock status screen
- `integration/` — `app_spec`, connectivity + diagnostics + storage, periodic tick for demo signals

## Run

From the **Blusys repo root**:

```bash
blusys host-build examples/reference/interactive_panel
# → examples/reference/interactive_panel/build-host/interactive_panel_host
blusys build examples/reference/interactive_panel esp32
```

See [Archetype Starters](../../../docs/start/archetypes.md) for controller vs panel choice.

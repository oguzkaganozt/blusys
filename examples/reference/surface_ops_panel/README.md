# Interactive panel (surface reference)

**Surface** interactive reference: denser, operational dashboard on the same reducer, shell, and capability stack as the handheld quickstart — proves reuse without a second architecture.

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

Device builds pick **ILI9341** or **ILI9488** from the Blusys framework display menu; host SDL matches via `BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE`. This is separate from the canonical ST7735 **handheld** path used by the handheld starter quickstart.

## Layout

- `core/` — sampled load history, mode, diagnostics/storage sync actions
- `ui/` — dashboard composition, stock status screen
- `integration/` — `app_spec`, connectivity + diagnostics + storage, periodic tick for demo signals

## Run

From the **Blusys repo root**:

```bash
blusys host-build examples/reference/surface_ops_panel
# → examples/reference/surface_ops_panel/build-host/surface_ops_panel_host
blusys build examples/reference/surface_ops_panel esp32
```

See [Product shape](../../../docs/start/product-shape.md) for handheld vs surface choice.

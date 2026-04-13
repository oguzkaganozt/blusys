# Archetype Starters

Blusys ships four canonical archetype starting points on the same `blusys::app` operating model. They are product starting shapes, not framework branches.

Create with `blusys create --archetype <interactive-controller|interactive-panel|edge-node|gateway-controller>`.

## Shared model

All four keep the same ownership rules and runtime model:

- `core/` — state, actions, reducer behavior
- `ui/` — screens and rendering (may be empty for headless)
- `integration/` — capabilities, profiles, `app_spec`, event wiring
- `update(ctx, state, action)` reducer, framework-owned shell and navigation, framework-owned profile bring-up, capability events mapped into app actions

## Interactive archetypes

The controller and panel share the same shell + capabilities model and differ mainly in **default theme, primary input, and data density**.

| Archetype | Product shape | Default theme | Inputs | Typical flows | Canonical example |
|-----------|---------------|---------------|--------|----------------|-------------------|
| **Interactive controller** | Compact, tactile, expressive | `expressive_dark` | Encoder primary; buttons; touch optional | Settings, status, setup/provisioning, short control loops | `examples/quickstart/interactive_controller/` (PR CI quickstart) |
| **Interactive panel** | Glanceable operator surface, denser data | `operational_light` | Touch, button row, encoder per bridges | Dashboard, diagnostics, connectivity + storage visibility | `examples/reference/interactive_panel/` (larger default display profile, ILI9341 / ILI9488; still builds on host SDL) |

Choose **controller** when the primary loop is “adjust and confirm” on a small display. Choose **panel** when operators need charts, tables, and system status at a glance on a larger surface.

Both ship `host/CMakeLists.txt`. From the repo root: `blusys host-build examples/quickstart/interactive_controller` or `blusys host-build examples/reference/interactive_panel` builds the full SDL + LVGL app under `build-host/`. `blusys host-build` with no project path builds the shared host smoke tests and widget kit in `scripts/host/`.

## Connected archetypes

Use when the product is connectivity-led, not display-led. Both use the same `blusys::app` reducer, capabilities, and `integration/` event bridges — no separate “connectivity framework.”

| Archetype | Choose when… | Canonical example |
|-----------|----------------|-------------------|
| **Edge node** | Headless-first; telemetry/OTA/provisioning central; minimal or no local UI; “invisible when healthy, obvious when broken.” | `examples/quickstart/edge_node/` |
| **Gateway / controller** | Coordinator or operator device; optional local panel; orchestration, diagnostics, and upstream link matter as much as local sensors. | `examples/reference/gateway/` |

**Headless-first (default for edge node):** smallest firmware surface; status via logs, local control JSON, and telemetry. Enable the optional SSD1306 path only when a tiny local status readout is worth the hardware.

**Gateway** defaults to an interactive ESP-IDF build to prove shell + dashboard + stock screens; on host, the same app runs under SDL with `BLUSYS_APP_MAIN_HOST_PROFILE`. For the local UI variant explicitly: `blusys create --archetype gateway-controller --starter interactive`.

### Operational runbook (connected products)

1. **First boot** — watch logs for provisioning phase; complete SoftAP/BLE provisioning or use pre-provisioned flash.
2. **Steady state** — confirm phase reaches `reporting` (edge) or `operational` (gateway); telemetry batches appear in delivery logs.
3. **Connectivity loss** — expect reconnect loops; telemetry resumes after IP returns (no separate code path; capabilities + reducer handle it).
4. **OTA** — trigger from your pipeline or a test endpoint; watch `updating` phase and completion/rollback logs.
5. **Health** — use diagnostics capability snapshots and local control JSON.

### Relation to older connectivity examples

- `examples/reference/connected_headless/` — minimal headless demo of connectivity + storage.
- `examples/reference/connected_device/` — minimal interactive + Wi-Fi status (historical).

For new products, start from **edge node** or **gateway**.

## Rules that apply to all archetypes

- **No raw LVGL** in normal product code: use `blusys::app::view`, stock widgets, and documented custom widget scope only.
- **Capabilities** are composed in `integration/`; the reducer requests work via actions; events map back to actions in `integration/`.
- These are **product-shaped references**, not generic demos. After fork, replace branding, copy, and domain state while keeping the structure and capability contract.

## Next pages

- [Interactive Quickstart](quickstart-interactive.md)
- [Headless Quickstart](quickstart-headless.md)
- [App](../app/index.md)
- [Capabilities](../app/capabilities.md)
- [Profiles](../app/profiles.md)

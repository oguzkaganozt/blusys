# Connected products (archetypes)

Use this page together with [Archetype Starters](archetypes.md) when the product is **connectivity-led** (not only display-led).

## When to use which archetype

| Archetype | Choose when… | Canonical example |
|-----------|----------------|-------------------|
| **Edge node** | Headless-first, telemetry/OTA/provisioning are central; minimal or no local UI; “invisible when healthy, obvious when broken.” | `examples/quickstart/edge_node/` |
| **Gateway / controller** | Coordinator or operator device; optional local panel; orchestration, diagnostics, and upstream link matter as much as local sensors. | `examples/reference/gateway/` |

Both use the same `blusys::app` reducer, capabilities, and `integration/` event bridges. Neither introduces a separate “connectivity framework.”

## Headless-first vs local UI

- **Headless-first** (default for edge node): smallest firmware surface; status via logs, local control JSON, and telemetry. Enable the optional SSD1306 path only when a tiny local status readout is worth the hardware.
- **Gateway** defaults to an **interactive** build on ESP-IDF to prove shell + dashboard + stock screens; on **host**, the same app runs under SDL with `BLUSYS_APP_MAIN_HOST_PROFILE`.

## Relation to older examples

- `examples/reference/connected_headless` — minimal single-file demo of connectivity + storage (historical).
- `examples/reference/connected_device` — minimal interactive + Wi-Fi status (historical).

For new products, start from **edge node** or **gateway** so provisioning, telemetry, OTA, diagnostics, and operational phases are already composed.

## Operational runbook (field-style)

1. **First boot** — Watch logs for provisioning phase; complete SoftAP/BLE provisioning or use pre-provisioned flash.
2. **Steady state** — Confirm phase reaches `reporting` (edge) or `operational` (gateway); telemetry batches appear in delivery logs (or your backend).
3. **Connectivity loss** — Expect reconnect loops; telemetry should resume after IP returns (no separate code path in app — capabilities + reducer).
4. **OTA** — Trigger from your pipeline or test endpoint; watch `updating` phase and completion/rollback logs.
5. **Health** — Use diagnostics capability snapshots and local control JSON for snapshot in time.

For locked state/action scope, see [Phase 6 archetype briefs](../internals/phase-6-archetype-briefs.md).

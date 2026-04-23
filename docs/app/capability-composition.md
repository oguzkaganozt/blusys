# Capability Composition

Wire capabilities in the app entrypoint (or a tiny `main/` helper); map integration events to `Action`s in `on_event`. [Product shape](../start/product-shape.md) and **`blusys create --list`** set manifest and dependencies. [Adding a flow](adding-a-flow.md) covers stock flows / `spec.flows` — not duplicated here.

## Rules

- Config and list live under `main/`; **reducer** owns product state; **UI** stays in `ui/`.
- **`on_event`** translates capability events; **`app_ctx`** / **`ctx.fx()`** for status and approved navigation.
- If a capability changes product behaviour, emit an **action**; if it only wires services, keep that wiring in a small helper.

## Typical stacks

Suggested **minimal** vs **full** sets (connected products use **`connectivity`** for Wi-Fi lifecycle):

### `interactive`

| Minimal | Full (typical) |
|---------|----------------|
| storage | storage, connectivity, diagnostics |

### `headless` (connected)

| Minimal | Full (typical) |
|---------|----------------|
| connectivity, telemetry | connectivity, telemetry, OTA, diagnostics, storage, lan_control |

### Interactive coordinator (local UI + connected)

| Minimal | Full (typical) |
|---------|----------------|
| connectivity, telemetry | connectivity, telemetry, OTA, diagnostics, storage, lan_control |

**Flows** (`blusys::flows::*`, `blusys::screens::*`) are LVGL-only: drive work through `app_ctx::dispatch` and [Adding a flow](adding-a-flow.md), not raw services from stock UIs.

???+ note "Event IDs, on_event, and typed helpers (reference)"
    Each capability posts integration events in a reserved range (see `blusys::capability.hpp`). In wiring, map IDs to `Action` values. Typical events:

    | Source | Typical events to surface |
    |--------|---------------------------|
    | Connectivity | `wifi_*`, `time_synced`, provisioning phases, `capability_ready` |
    | Storage | `spiffs_mounted`, `fatfs_mounted`, `capability_ready` |
    | OTA | `download_*`, `apply_failed`, `rollback_pending`, `capability_ready` |
    | Diagnostics | `snapshot_ready`, `capability_ready` |
    | Telemetry | `buffer_flushed`, `delivery_ok` / `delivery_failed`, `capability_ready` |
    | Bluetooth | `gap_ready`, `gatt_ready`, `client_connected`, `capability_ready` |

    Status-only updates can be dropped in `on_event` if `app_ctx` queries are enough.

    - Use `blusys::integration::*` (`as_*_event`, `kind_for_event_id`, …) to keep `on_event` small.
    - Optional: `std::variant<...>` for `Action` + `blusys::dispatch_variant` (`variant_dispatch.hpp`).

## Reference `on_event` Implementation

For a full reducer-facing translation table, see the headless validation example under `examples/validation/connected_headless/` — it composes all major capabilities (connectivity, storage, OTA, diagnostics, telemetry, lan_control) and shows the complete event handling pattern.

Keep product-specific tags in reducer-facing code; only the app wiring layer should map raw integration event IDs to those tags.

On Linux, `blusys host-build` (monorepo default) places binaries under `scripts/host/build-host/` — including `operational_phase_smoke` (phase machine parity) and `connected_headless_host` (headless reducer + capability stubs). See `scripts/host/README.md`.

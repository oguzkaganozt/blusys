# Capability Composition

Capabilities are composed in `platform/` and translated into app actions.

## Composition Rules

- keep capability config in `platform/`
- keep product decisions in `core/`
- keep rendering in `ui/`
- map capability events into reducer actions in `platform/`
- query capability status through `app_ctx` (or `ctx.services()` only when routing/UI reactions are needed)

## Typical stacks

Each **interface** suggests a different **minimal** capability set (enough for a credible starter) and a **full** stack for a typical connected product. Compose capabilities only in `platform/`; onboarding and Wi-Fi lifecycle live under **`connectivity`**. See [Product shape](../start/product-shape.md) and **`blusys create --list`** for dependency rules.

### Handheld (`--interface handheld`)

| Minimal | Full (typical) |
|---------|----------------|
| storage | storage, connectivity (optional) |

Starter: `examples/quickstart/handheld/`.

### Surface (`--interface surface`)

| Minimal | Full (typical) |
|---------|----------------|
| connectivity | connectivity, storage, diagnostics |

Starter: `examples/quickstart/handheld/`.

### Headless connected (`--interface headless` + connected `--with`)

| Minimal | Full (typical) |
|---------|----------------|
| connectivity, telemetry | connectivity, telemetry, OTA, diagnostics, storage, lan_control |

Starter: `examples/quickstart/headless/`.

### Surface coordinator

| Minimal | Full (typical) |
|---------|----------------|
| connectivity, telemetry | connectivity, telemetry, OTA, diagnostics, storage, lan_control |

Starter: `examples/quickstart/headless/` (optional local UI depends on build/profile; scaffold selects interface explicitly).

## Rule Of Thumb

If a capability changes app behavior, emit an action.
If it only wires runtime services, keep it in `platform/`.

## Event IDs And `map_event`

Each capability posts integration events in a reserved numeric range (see `blusys::capability.hpp`). In `platform/`, implement `map_event` to translate those IDs into product `Action` values. Prefer handling at least:

| Source | Typical events to surface |
|--------|---------------------------|
| Connectivity | `wifi_*`, `time_synced`, onboarding/provisioning phases (`already_provisioned`, `credentials_received`, `success`, `failed`, …), `capability_ready` |
| Storage | `spiffs_mounted`, `fatfs_mounted`, `capability_ready` |
| OTA | `download_started`, `download_progress` (event `code` = percent 0–100), `download_complete`, `apply_failed`, `rollback_pending`, `capability_ready` |
| Diagnostics | `snapshot_ready`, `capability_ready` |
| Telemetry | `buffer_flushed`, `delivery_ok`, `delivery_failed`, `capability_ready` |
| Bluetooth | `gap_ready`, `gatt_ready`, `client_connected`, `capability_ready` |

Status-only updates can be ignored in the reducer if `app_ctx` queries are enough.

### Typed helpers and `std::variant` actions

- Use `blusys::integration::*` (`as_*_event`, `kind_for_event_id`, and predicates such as `event_is_diagnostics_snapshot_or_ready`) to keep `map_event` small without hiding the translation table.
- For products that want exhaustiveness on actions, the optional path is `std::variant<...>` for `Action` plus `blusys::dispatch_variant` (`blusys/app/integration.hpp` / `variant_dispatch.hpp`). The default remains a plain tagged struct or enum.

## Stock Flows And Screens

Operational UI (`blusys::flows::*`, `blusys::screens::*`) stays LVGL-backed and must not call runtime services directly. Wire buttons through `app_ctx::dispatch`, `error_panel_dispatch`, `confirmation_dispatch`, or `app_ctx::request_connectivity_reconnect()` as appropriate; use `ctx.services()` only for framework-approved navigation/routing from UI callbacks that already hold `app_ctx`.

## Reference `map_event` Implementations

For a full reducer-facing translation table, see `map_event` in:

- `examples/quickstart/headless/main/platform/app_main.cpp` (headless-first, all seven capabilities)
- `examples/quickstart/headless/main/platform/app_main.cpp` (connected stack entry)

Keep product-specific tags in `core/`; only the integration layer should map raw integration event IDs to those tags.

On Linux, `blusys host-build` (monorepo default) places binaries under `scripts/host/build-host/` — including `operational_phase_smoke` (phase machine parity) and `connected_headless_host` (headless reducer + capability stubs). See `scripts/host/README.md`.

# Capability Composition

Capabilities are composed in the app entrypoint or a small product-local wiring file and translated into app actions.

## Composition Rules

- keep capability config in the app entrypoint or a tiny wiring helper under `main/`
- keep product decisions in the reducer
- keep rendering in `ui/`
- handle capability events in `on_event`
- query capability status through `app_ctx` (use `ctx.fx()` for routing/UI reactions when needed)

## Typical stacks

Each **interface** suggests a different **minimal** capability set (enough for a credible starter) and a **full** stack for a typical connected product. Compose capabilities in product-local wiring; onboarding and Wi-Fi lifecycle live under **`connectivity`**. The starter docs in `docs/start/quickstart-interactive.md` and `docs/start/quickstart-headless.md` describe the target manifest-first app shape. See [Product shape](../start/product-shape.md) and **`blusys create --list`** for dependency rules.

### Interactive (`--interface interactive`)

| Minimal | Full (typical) |
|---------|----------------|
| storage | storage, connectivity, diagnostics |

Starter: manifest-first interactive starter.

### Headless connected (`--interface headless` + connected `--with`)

| Minimal | Full (typical) |
|---------|----------------|
| connectivity, telemetry | connectivity, telemetry, OTA, diagnostics, storage, lan_control |

Starter: manifest-first headless starter.

### Interactive coordinator

| Minimal | Full (typical) |
|---------|----------------|
| connectivity, telemetry | connectivity, telemetry, OTA, diagnostics, storage, lan_control |

Starter: manifest-first interactive starter with optional local UI.

## Rule Of Thumb

If a capability changes app behavior, emit an action.
If it only wires runtime services, keep it in a small wiring helper under `main/`.

## Event IDs And `on_event`

Each capability posts integration events in a reserved numeric range (see `blusys::capability.hpp`). In the app wiring layer, implement `on_event` to translate those IDs into product `Action` values. Prefer handling at least:

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

- Use `blusys::integration::*` (`as_*_event`, `kind_for_event_id`, and predicates such as `event_is_diagnostics_snapshot_or_ready`) to keep `on_event` small without hiding the translation table.
- For products that want exhaustiveness on actions, the optional path is `std::variant<...>` for `Action` plus `blusys::dispatch_variant` (`blusys/framework/app/variant_dispatch.hpp`). The default remains a plain tagged struct or enum.

## Stock Flows And Screens

Operational UI (`blusys::flows::*`, `blusys::screens::*`) stays LVGL-backed and must not call runtime services directly. Wire buttons through `app_ctx::dispatch`, `error_panel_dispatch`, `confirmation_dispatch`, or `app_ctx::request_connectivity_reconnect()` as appropriate; use `ctx.fx()` only for framework-approved navigation/routing from UI callbacks that already hold `app_ctx`.

## Reference `on_event` Implementation

For a full reducer-facing translation table, see the headless validation example under `examples/validation/connected_headless/` — it composes all major capabilities (connectivity, storage, OTA, diagnostics, telemetry, lan_control) and shows the complete event handling pattern.

Keep product-specific tags in reducer-facing code; only the app wiring layer should map raw integration event IDs to those tags.

On Linux, `blusys host-build` (monorepo default) places binaries under `scripts/host/build-host/` — including `operational_phase_smoke` (phase machine parity) and `connected_headless_host` (headless reducer + capability stubs). See `scripts/host/README.md`.

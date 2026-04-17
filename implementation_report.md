# Implementation Report

## Implemented

- P3 typed effects are in place. `app_fx` is the reducer-facing surface, and the earlier validation stayed green.
- P4 one event pipeline is now wired through the framework.
- `app_spec` exposes only `on_event` for event handling.
- `app_runtime` now forwards framework events directly to `on_event`.
- The old `map_intent`, `map_event`, `capability_event_discriminant`, and `integration_dispatch.hpp` path has been removed from code.
- Product examples and host demos were migrated to `on_event`.
- The scaffold starter and generator were updated to emit the unified event hook.

## Validation Status

- `./blusys build examples/quickstart/bluetooth_controller esp32s3` passes.
- `./blusys build examples/validation/connected_headless esp32s3` passes.
- `./blusys host-build` currently fails on `scripts/host/src/connected_headless_host.cpp`, which still expects the old capability-event bridge.

## Still Needed According To `IMPLEMENTATION_PLAN.md`

- Finish removing the last stale host/test code path in `scripts/host/src/connected_headless_host.cpp`.
- Decide whether to clean the remaining docs that still mention `map_intent` / `map_event`.
- Continue with P5, then the later phases:
  - P5 capability shell + access contract
  - P6 UI as a reactive tree
  - P7 entry consolidation + flows as spec inputs
  - P8 display flush cleanup
  - P9 public API lockdown + docs
  - P10 persistence & configuration
  - P11 observability maturity
  - P12 capability scaffolder + external-shape validation

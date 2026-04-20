# Validation and host iteration

This page maps the **validation and host-iteration** expectations from the repository **[README](https://github.com/oguzkaganozt/blusys/blob/main/README.md)** (**Product foundations** → **Validation**) to concrete tools in this repository. It is the recommended place to answer “how do we prove the product path without flashing hardware every time?”

## Requirements

| Goal | What we use |
|-----------------|-------------|
| **Reducer tests runnable on host** | `scripts/host-test/` (`host_harness_smoke`, `session_helper_smoke`) plus the shared headless helpers they exercise. |
| **Capability integration tests with simulated runtime-service events** | `scripts/host/` binaries such as `capability_contract_smoke`, `handheld_starter_reducer_smoke`, `panel_connectivity_map_smoke`, `operational_phase_smoke`, `host_spine_test`, `action_queue_drop_smoke`, `snapshot_buffer_smoke`, and `connected_headless_host`. |
| **Fast repo preflight** | `blusys validate` (manifest checks, inventory, product-layout, and lint in one pass). |
| **Inventory-driven example builds** | `blusys build-inventory <target> <ci_pr|ci_nightly>` (same inventory filter CI uses). |
| **Screenshot or visual smoke where practical** | `widget_kit_demo`, `app_interactive_demo`, and host builds of interactive examples (SDL). Not automated as image snapshots in CI today. |
| **Scaffold smoke for generated starters** | `scripts/scaffold-smoke.sh` (also run in CI after host smokes). |
| **Curated CI aligned with the canonical product path** | `.github/workflows/ci.yml` — layering lint, inventory, host smokes, docs strict build, `build-from-inventory.sh` / `blusys build-inventory` for `ci_pr=true` examples, multi-profile display builds (e.g. ST7735 variants), QEMU subset. |
| **Fast host iteration** | `blusys host-build` for projects with `host/CMakeLists.txt`; monorepo `blusys host-build` (no path) builds `scripts/host` harnesses. |

`blusys validate` and `blusys build-inventory` need PyYAML (`pip install pyyaml`).

## Host harness smokes (quick reference)

From the repo root, after `blusys host-build` (no project path), interactive binaries live under `scripts/host/build-host/`; minimal headless tests live under `build-host-test/` (or `build-host-ci` in automation). Typical local run:

```bash
blusys validate
blusys host-build
./scripts/host/build-host/handheld_starter_reducer_smoke
./scripts/host/build-host/operational_phase_smoke
./scripts/host/build-host/capability_contract_smoke
./scripts/host/build-host/panel_connectivity_map_smoke
ctest --test-dir build-host-test --output-on-failure -R host_harness_smoke
ctest --test-dir build-host-test --output-on-failure -R session_helper_smoke
```

PR CI runs the same `scripts/host/` smoke binaries in `host-smoke` and the two `scripts/host-test/` ctest targets in `host-test-minimal`.

## Scaffold validation

```bash
./scripts/scaffold-smoke.sh
```

Creates temporary projects across the interface/capability/policy matrix and runs `blusys host-build` on each. Uses `FETCHCONTENT_BASE_DIR` so interactive host builds share one LVGL fetch.

## Where to add new tests

- **Pure reducer / state machine logic:** prefer small `add_executable` targets in `scripts/host/CMakeLists.txt` with no device dependencies — follow `operational_phase_smoke.cpp` or `handheld_starter_reducer_smoke.cpp`.
- **Capability event mapping:** extend `capability_contract_smoke` for static range checks; use `panel_connectivity_map_smoke`-style tests for `on_event` tables; use `connected_headless_host` pattern for end-to-end stub capability polling (run manually — entry uses the headless runtime loop).
- **Product-specific logic:** keep tests next to the example or under `scripts/host` if shared across multiple examples.

## Related

- [Device, host & QEMU CLI](cli-host-qemu.md) — single-page `blusys build` / `blusys qemu` matrix (chip, `host`, `qemu*`, `--graphics`)
- [Reducer Model](reducer-model.md)
- [Capability Composition](capability-composition.md)
- `scripts/host/README.md` — install and binary list
- [README](https://github.com/oguzkaganozt/blusys/blob/main/README.md) (**Product foundations**) — mission, constraints, and validation pointer

# Validation and host iteration

This page maps the **validation and host-iteration** expectations from the repository **[README](https://github.com/oguzkaganozt/blusys/blob/main/README.md)** (**Product foundations** → **Validation**) to concrete tools in this repository. It is the recommended place to answer “how do we prove the product path without flashing hardware every time?”

## Requirements

| Goal | What we use |
|-----------------|-------------|
| **Reducer tests runnable on host** | Small host executables under `scripts/host/` (see below) plus archetype-shared helpers tested by `interactive_archetype_reducer_smoke` and edge-node-style logic in `operational_phase_smoke`. |
| **Capability integration tests with simulated runtime-service events** | `capability_contract_smoke` (event ID bands), `panel_connectivity_map_smoke` (connectivity `map_event` path), and the **`connected_headless_host`** harness (full headless `app_spec` + capability host stubs — run locally; see `scripts/host/README.md`). |
| **Screenshot or visual smoke where practical** | `widget_kit_demo`, `app_interactive_demo`, and host builds of archetype examples (SDL). Not automated as image snapshots in CI today. |
| **Scaffold smoke for generated starters** | `scripts/scaffold-smoke.sh` (also run in CI after host smokes). |
| **Curated CI aligned with the canonical product path** | `.github/workflows/ci.yml` — layering lint, inventory, host smokes, docs strict build, `build-from-inventory.sh` for `ci_pr=true` examples, multi-profile display builds (e.g. ST7735 variants), QEMU subset. |
| **Fast host iteration** | `blusys host-build` for projects with `host/CMakeLists.txt`; monorepo `blusys host-build` (no path) builds `scripts/host` harnesses. |

## Host harness smokes (quick reference)

From the repo root, after `blusys host-build` (no project path), binaries live under `scripts/host/build-host/` (or `build-host-ci` in automation). Typical local run:

```bash
blusys host-build
./scripts/host/build-host/interactive_archetype_reducer_smoke
./scripts/host/build-host/operational_phase_smoke
./scripts/host/build-host/capability_contract_smoke
./scripts/host/build-host/panel_connectivity_map_smoke
```

PR CI runs the same four executables (see `host-smoke` job in `.github/workflows/ci.yml`).

## Scaffold validation

```bash
./scripts/scaffold-smoke.sh
```

Creates temporary projects for all archetypes (plus gateway `--starter interactive`) and runs `blusys host-build` on each. Uses `FETCHCONTENT_BASE_DIR` so interactive host builds share one LVGL fetch.

## Where to add new tests

- **Pure reducer / state machine logic:** prefer small `add_executable` targets in `scripts/host/CMakeLists.txt` with no device dependencies — follow `operational_phase_smoke.cpp` or `interactive_archetype_reducer_smoke.cpp`.
- **Capability event mapping:** extend `capability_contract_smoke` for static range checks; use `panel_connectivity_map_smoke`-style tests for `map_event` tables; use `connected_headless_host` pattern for end-to-end stub capability polling (run manually — entry uses the headless runtime loop).
- **Product-specific logic:** keep tests next to the example or under `scripts/host` if shared across archetypes.

## Related

- [Reducer Model](reducer-model.md)
- [Capability Composition](capability-composition.md)
- `scripts/host/README.md` — install and binary list
- [README](https://github.com/oguzkaganozt/blusys/blob/main/README.md) (**Product foundations**) — mission, constraints, and validation pointer

---
title: Validation and host loop
---

# Validation and host loop

Use this page when you want the repo commands that match CI and local smoke coverage.

## First run

1. `blusys validate`
2. `blusys host-build`

## What each command covers

| Command | Purpose |
|---------|---------|
| `blusys validate` | manifests, inventory, layout, lint, and generated-artifact drift |
| `blusys host-build` | host binaries for project apps or the monorepo harness |
| `blusys build-inventory <target> <ci_pr|ci_nightly>` | inventory-filtered example builds |
| `scripts/scaffold-smoke.sh` | generated starter smoke across the shape matrix |

## Host smokes

After `blusys host-build`, the binaries live under `scripts/host/build-host/` and `build-host-test/`.

```bash
blusys validate
blusys host-build
./scripts/host/build-host/interactive_starter_reducer_smoke
./scripts/host/build-host/operational_phase_smoke
ctest --test-dir build-host-test --output-on-failure -R host_harness_smoke
```

## Where to add tests

- Reducer logic: `scripts/host/CMakeLists.txt`
- Capability event mapping: `capability_contract_smoke` or `panel_connectivity_map_smoke`
- Product-specific logic: next to the example or under `scripts/host`

## Related

- [Device, host & QEMU CLI](cli-host-qemu.md)
- [Capability composition](capability-composition.md)
- [README](https://github.com/oguzkaganozt/blusys/blob/main/README.md)

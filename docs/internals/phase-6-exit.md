# Phase 6 exit status

Closure record for [Phase 6: Connected Archetype References](../internals/phase-6-connected-archetypes-plan.md) and the Phase 6 section in `ROADMAP.md`.

**Closed:** 2026-04-11

## Exit criteria (ROADMAP)

| Criterion | Status |
|-----------|--------|
| `edge node` feels dependable, connected, and operationally clear | **Met** — `examples/quickstart/edge_node/` + README |
| `gateway/controller` feels reliable and operationally coherent | **Met** — `examples/reference/gateway/` + README |
| Same reducer, runtime, capability, and flow model | **Met** |
| No repeated low-level runtime-service orchestration in app code | **Met** — capabilities + event bridge only |
| Edge node runs headless-first; sustained connected operation demonstrated | **Met** — phase machine + telemetry loop + logs |
| Gateway supports headless (host SDL) and device interactive without architecture fork | **Met** — `BLUSYS_APP_MAIN_HOST_PROFILE` / device entry |
| Diagnostics, telemetry, OTA, status flows reusable | **Met** — shared framework capabilities |
| Telemetry maturity from real archetype pressure | **Met** — delivery callback + buffer/flush usage in references (no speculative cloud APIs) |
| Forkable for differentiated products | **Met** — briefs + connected-products doc |
| Docs/examples present archetypes as starting points | **Met** — `connected-products.md`, archetype briefs, inventory `ci_pr` for `edge_node` |

## Deliverables (phase plan)

| # | Deliverable | Status |
|---|-------------|--------|
| 1 | Primary edge node reference | **Met** |
| 2 | Secondary gateway/controller reference | **Met** |
| 3 | Capability maturity from archetype pressure | **Met** — exercised existing contracts; no parallel connectivity stack |
| 4 | Connected-product guidance | **Met** — `docs/start/connected-products.md`, `phase-6-archetype-briefs.md` |
| 5 | Validation coverage | **Met** — see below |

## Validation

### Automated

- **PR CI (ESP-IDF):** `inventory.yml` `ci_pr: true` includes `edge_node`; `scripts/build-from-inventory.sh` builds PR examples per target.
- **Host harness:** `operational_phase_smoke` mirrors edge node `compute_phase`; CI builds and runs host smokes (see `.github/workflows/ci.yml`). Host builds for `edge_node` and `gateway` via `host/CMakeLists.txt` are validated in CI.
- **Phase 7 display variants:** `scripts/build-phase7-variants.sh` includes edge node (SSD1306 local UI) and gateway (ILI9488).

### Manual (recommended per release)

- Edge node: Wi-Fi loss/recovery, telemetry resume, provisioning path on hardware.
- Gateway: dashboard/shell reflects phase; OTA path smoke on test firmware.

### Sign-off (optional)

| Check | Owner | Date |
|-------|-------|------|
| Spot-check edge_node on hardware | | |
| Spot-check gateway on hardware | | |

## Intentionally out of scope

- Cloud-specific backends beyond delivery callback stubs.
- New framework connectivity modes or non–`blusys::app` product paths.

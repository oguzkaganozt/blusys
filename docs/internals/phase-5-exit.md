# Phase 5 exit status

Closure record for [Phase 5: Interactive Archetype References](../internals/phase-5-interactive-archetypes-plan.md) and the Phase 5 section in `ROADMAP.md`.

**Closed:** 2026-04-11

## Exit criteria (ROADMAP)

| Criterion | Status |
|-----------|--------|
| `interactive controller` feels tactile, expressive, and productized | **Met** — `examples/quickstart/interactive_controller` + README brief |
| `interactive panel` feels coherent and product-ready | **Met** — `examples/reference/interactive_panel` + README brief |
| Same reducer, runtime, shell, capability, and flow model | **Met** |
| No raw LVGL in normal app code | **Met** — composition via framework views / stock APIs |
| Forkable without restructuring | **Met** — documented in [Interaction design](../start/interaction-design.md) |
| Docs and examples present archetypes as starting points | **Met** — `docs/start/archetypes.md`, `interaction-design.md`, quickstart links |

## Deliverables (phase plan)

| # | Deliverable | Status |
|---|-------------|--------|
| 1 | Primary interactive controller reference | **Met** |
| 2 | Secondary interactive panel reference | **Met** |
| 3 | Framework refinements from archetype pressure | **Met** — shared helpers under `examples/include/blusys_examples/` where reuse removes drift |
| 4 | Interaction and design guidance | **Met** — `docs/start/interaction-design.md` |
| 5 | Host + device validation | **Met** — see below |

## Validation

### Automated

- **PR CI (ESP-IDF):** `inventory.yml` entries with `ci_pr: true` include `interactive_controller` and `interactive_panel`; `scripts/build-from-inventory.sh` runs on `esp32`, `esp32c3`, `esp32s3`.
- **Phase 7 display variants:** `scripts/build-phase7-variants.sh` covers both archetypes (ST7789 / ILI9488 matrix).
- **Host harness (Linux):** CI job `host-smoke` builds `scripts/host/` and runs:
  - `interactive_archetype_reducer_smoke` (shared `clamp_percent` with archetype reducers)
  - `panel_connectivity_map_smoke` (panel connectivity `map_event` bridge)
  - `capability_contract_smoke`

### Manual (recommended per release)

- Focus traversal and encoder/touch parity on hardware for both references.
- Confirm/cancel and overlay behavior on controller; status screen refresh on panel after connectivity events.

### Sign-off (optional)

| Check | Owner | Date |
|-------|-------|------|
| Spot-check interactive_controller on hardware (focus, feedback, provisioning path) | | |
| Spot-check interactive_panel on hardware (dashboard, status, Wi‑Fi path if used) | | |

## Intentionally out of scope

- Screenshot automation in CI (optional future work).

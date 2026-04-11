# Phase 8 exit status

Closure record for [Phase 8: Ecosystem, Archetype Scaffolds, And Docs](../internals/phase-8-ecosystem-archetype-scaffolds-docs-plan.md) and the Phase 8 section in `ROADMAP.md`.

**Closed:** 2026-04-11

## Exit criteria (ROADMAP)

| Criterion | Status |
|-----------|--------|
| A new developer can identify the right product family quickly | **Met** — `docs/start/index.md` leads with archetypes; `blusys create --list-archetypes`; `docs/start/archetypes.md` |
| A new product starts from an archetype, not a blank technical shell | **Met** — `blusys create --archetype …` copies curated examples; README generated per project |
| Archetype choice changes defaults and starter content, not layout | **Met** — fixed `main/core`, `main/ui` (when used), `main/integration` across archetypes |
| The public repo surface reflects the platform mindset consistently | **Met** — Start → App docs, widget gallery + capability composition tables, CI scaffold smoke |

## Deliverables (ROADMAP)

| Deliverable | Status |
|-------------|--------|
| Starter compositions for four canonical archetypes | **Met** — `examples/quickstart/interactive_controller`, `examples/reference/interactive_panel`, `examples/quickstart/edge_node`, `examples/reference/gateway` + `blusys create` |
| Same `core/` / `ui/` / `integration/` structure | **Met** — scaffold enforces layout; gateway headless drops `main/ui` via template |
| Same `main/` single local component | **Met** — unchanged |
| Docs: four archetypes + consumer/industrial lenses | **Met** — `archetypes.md`, `interaction-design.md`, `connected-products.md`, `index.md` funnel |
| Widget gallery and capability composition guides | **Met** — `docs/app/widget-gallery.md`, `docs/app/capability-composition.md` (minimal vs full stacks) |
| Quickstart / scaffold centered on archetypes | **Met** — CLI help + `--list-archetypes`; `docs/start/index.md` |

## Implementation notes

- **Gateway headless scaffold** uses checked-in files under `templates/scaffold/gateway_headless/` (main CMake, integration entry, host CMake) instead of inline shell heredocs in `blusys`.
- **`--starter`** remains a documented legacy alias when `--archetype` is omitted; prefer explicit `--archetype`.

## Validation

### Automated

- **Scaffold smoke:** `scripts/scaffold-smoke.sh` — creates five projects (four archetypes + gateway with `--starter interactive`) and runs `blusys host-build` on each. Wired in `.github/workflows/ci.yml` (`host-smoke` job). Uses `FETCHCONTENT_BASE_DIR` so interactive host builds share one LVGL fetch.
- **Docs:** `mkdocs build --strict` (CI `docs` job).
- **Inventory:** `python scripts/check-inventory.py` (CI `inventory` job).

### Manual (optional)

- Run `blusys create --archetype <name> <path>` on a clean directory and confirm the README matches the chosen archetype.
- Spot-check `blusys build` for a generated project on hardware when changing scaffold templates.

## Intentionally deferred

- Full ESP-IDF `blusys build` for every generated project in CI (device coverage remains via curated `examples/` and `build-from-inventory.sh`).
- Large CLI rewrite (still out of scope per `ROADMAP.md`).

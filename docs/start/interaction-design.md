# Interaction design for interactive archetypes

This page complements [Archetype Starters](archetypes.md) with **interaction and presentation guidance** for the two Phase 5 interactive references. Both use the same `blusys::app` model: `core/` / `ui/` / `integration/`, reducer-driven actions, stock widgets, and capabilities composed in `integration/`.

## When to fork which archetype

| Start from | Product shape | Default theme | Inputs | Typical flows |
|------------|---------------|---------------|--------|----------------|
| **Interactive controller** (`examples/quickstart/interactive_controller/`, see `README.md` in-tree) | Compact, tactile, encoder-first control | `expressive_dark` | Encoder primary; buttons; touch optional | Settings, status, setup/provisioning, short control loops |
| **Interactive panel** (`examples/reference/interactive_panel/`, see `README.md` in-tree) | Glanceable operator surface, denser data | `operational_light` | Touch, button row, encoder per bridges | Dashboard, diagnostics, connectivity + storage visibility |

Choose **controller** when the primary loop is “adjust and confirm” on a small display. Choose **panel** when operators need charts, tables, and system status at a glance on a larger surface.

## Shared rules

- **No raw LVGL** in normal product code: use `blusys::app::view`, stock widgets, and documented custom widget scope only.
- **Capabilities** are composed in `integration/`; the reducer requests work via actions; events map to actions in `integration/`.
- **Host iteration:** each Phase 5 reference includes `host/CMakeLists.txt`. From the repo root, run `blusys host-build examples/quickstart/interactive_controller` or `blusys host-build examples/reference/interactive_panel` for the full SDL + LVGL app (binary under that example’s `build-host/`). The monorepo **`scripts/host`** harness (`blusys host-build` with no project path) builds shared smoke tests and the widget kit; PR CI runs the Phase 5 smokes there plus ESP-IDF builds for both archetypes.

## Validation expectations

Phase 5 treats these apps as **product-shaped references**, not generic demos. After fork, teams replace branding, copy, and domain state while keeping the same structure and capability contract.

See [Phase 5 exit](../internals/phase-5-exit.md) for the closure checklist and CI coverage.

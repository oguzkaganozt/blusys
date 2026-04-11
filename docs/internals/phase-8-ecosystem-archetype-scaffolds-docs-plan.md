# Phase 8 Ecosystem, Archetype Scaffolds, And Docs Plan

This document turns the Phase 8 target from `ROADMAP.md` into an execution-ready implementation plan for the current repo state.

Use it together with:

- `PRD.md` for product requirements and locked decisions
- `ROADMAP.md` for sequencing, dependencies, and phase exit criteria
- `docs/internals/architecture.md` for tier and ownership constraints
- `docs/internals/guidelines.md` for support-tier and validation rules
- `docs/internals/phase-5-interactive-archetypes-plan.md` for the interactive archetype baseline
- `docs/internals/phase-6-connected-archetypes-plan.md` for the connected archetype baseline
- `docs/internals/phase-7-hardware-packaging-plan.md` for the validated profile and packaging baseline

## Phase Goal

Make the proven platform discoverable directly from `blusys create` and easy to adopt without repo archaeology.

Phase 8 packages what earlier phases already validated. It does not add a new runtime model or a new archetype branch system.

## Phase Constraints

The following decisions are already locked by `PRD.md` and `ROADMAP.md` and must not be reopened during Phase 8:

- top-level runtime modes remain `interactive` and `headless`
- archetypes stay starter compositions, not framework branches
- product-facing code stays reducer-driven: `update(ctx, state, action)`
- product-facing namespace remains `blusys::app`
- capabilities are composed in `integration/`, not in `core/` or `ui/`
- scaffold and product structure stay `main/`, `core/`, `ui/`, and `integration/`
- `main/` remains the single local component for scaffolded product apps
- raw LVGL is allowed only inside custom widget implementations or explicit bounded custom view scope
- the three-tier architecture stays intact

## Current Repo Baseline

The repo already contains the validated archetype and packaging material that Phase 8 should package:

- `blusys::app` runtime, reducer model, profiles, capabilities, shell, widgets, and flows are real
- the canonical interactive archetypes exist as example references
- the canonical connected archetypes exist as example references
- the ST7735, ST7789, SSD1306, ILI9341, and ILI9488 profile surfaces already exist
- docs already explain the product model, but the onboarding story is still split across generic quickstarts and legacy terms
- `blusys create` still exposes a mode-first scaffold contract instead of an archetype-first one

The main gap is not missing platform architecture. The main gap is missing packaging: a clean front door, clear archetype guidance, and a public surface that makes the proven product shapes obvious.

## Phase Deliverables

Phase 8 should produce the following outputs:

1. Starter compositions for the four canonical archetypes.
2. Archetype-first `blusys create` behavior.
3. Docs paths that explain the four archetypes and the shared model clearly.
4. Widget gallery and capability composition guidance.
5. Validation coverage that proves generated starters and docs stay aligned.

## Archetype Briefs

### Interactive Controller

- mode bias: interactive
- input bias: encoder-first, buttons where appropriate, touch optional
- presentation bias: expressive, tactile, compact
- default theme direction: `expressive_dark`
- flow bias: setup, settings, compact control, status
- default profile family: ST7735 with ST7789 as the larger companion path

### Interactive Panel

- mode bias: interactive
- input bias: touch, button array, or encoder where appropriate
- presentation bias: operational, readable, denser
- default theme direction: `operational_light`
- flow bias: dashboard, diagnostics, settings, local control
- default profile family: ILI9341 with ILI9488 as the larger companion path

### Edge Node

- mode bias: headless-first
- input bias: no local UI by default; optional tiny status surface only when needed
- presentation bias: operational and minimal
- default theme direction: `operational_light` for the optional local surface
- flow bias: provisioning, connectivity lifecycle, telemetry, diagnostics, OTA
- default profile family: headless by default, SSD1306 for optional status surface

### Gateway/Controller

- mode bias: headless by default, with an optional local operator surface
- input bias: optional panel, encoder, or button surface where present
- presentation bias: operational, dashboard-oriented, clear status hierarchy
- default theme direction: `operational_light`
- flow bias: orchestration, connectivity, diagnostics, settings, maintenance
- default profile family: headless by default, ILI9341 or ILI9488 for optional local UI

## Workstreams

### Workstream A: Archetype-First Scaffold Contract

Define the starter contract around archetypes instead of runtime mode.

Required outcomes:

- `blusys create` accepts an archetype selector for the four canonical starting shapes
- the archetype determines the starter content and recommended defaults
- the generated project layout stays fixed across all archetypes
- the scaffold keeps `main/` as the single local component

### Workstream B: Scaffold Template Packaging

Move the recommended starter material out of inline shell text and into real files.

Required outcomes:

- starter files are sourced from real template files or validated example trees
- shared scaffold boilerplate is minimized
- archetype-specific starter content stays easy to audit and update
- README and first-run guidance are generated with the starter

### Workstream C: Public Docs Cutover

Update the public docs so the archetypes are the first thing a new developer sees.

Required outcomes:

- `Start` introduces the four archetypes clearly
- archetype selection guidance explains when to start from each shape
- the app surface points to archetype examples first
- capability and widget guidance are surfaced next to the archetype story

### Workstream D: Inventory And Nav Alignment

Keep docs inventory and site navigation consistent with the new onboarding story.

Required outcomes:

- new docs are listed in `inventory.yml`
- new docs are added to `mkdocs.yml`
- legacy references are demoted out of the recommended path

### Workstream E: Validation And Discoverability

Prove the new front door works in practice.

Required outcomes:

- scaffold smoke for all four archetypes
- host builds for the generated starters
- docs build with strict navigation validation
- inventory validation for docs/example coverage

## Proposed Repository Landing Pattern

Recommended starter mapping:

- `examples/quickstart/interactive_controller/` for `interactive controller`
- `examples/reference/interactive_panel/` for `interactive panel`
- `examples/quickstart/edge_node/` for `edge node`
- `examples/reference/gateway/` for `gateway/controller`

Starter selection rule:

- use `quickstart` for the canonical onboarding archetypes
- use `reference` for the secondary archetypes unless they become the primary onboarding path later

The implementation should keep the shared project structure stable while changing the starter content, defaults, and docs.

## Milestone Order

The recommended execution order for Phase 8 is:

1. Add the archetype-first scaffold contract to `blusys create`.
2. Package the starter content as real files or curated template sources.
3. Update the start and app docs to the archetype story.
4. Add the Phase 8 docs page to site navigation and inventory.
5. Validate generated starters and docs together.

## Validation Plan

Phase 8 validation should prove that the front door matches the documented model.

Required validation:

- `blusys create` for each canonical archetype
- `blusys host-build` for the generated starters where applicable
- targeted `blusys build` coverage for the generated starters on the supported targets
- `mkdocs build --strict`
- `python scripts/check-inventory.py`

Recommended manual checklist:

- the archetype choice is obvious at the point of creation
- the generated layout stays the same across all starters
- the README tells a new developer where to edit first
- the docs point to the archetype that matches the product family

## Exit Criteria

Phase 8 is complete when all of the following are true:

- a new developer can identify the right product family quickly
- a new product starts from an archetype, not from a blank technical shell
- the archetype choice changes defaults and starter content, not the project layout
- the public repo surface reflects the platform mindset consistently
- the docs, inventory, and scaffold story all agree on the same four archetypes

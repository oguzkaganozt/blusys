# Phase 0 V7 Foundation Plan

**Closure:** see [Phase 0 Exit](phase-0-exit.md) for the checklist against this plan.

This document turns the Phase 0 target from `ROADMAP.md` into an execution-ready implementation plan for the current repo state.

Use it together with:

- `PRD.md` for product requirements and locked decisions
- `ROADMAP.md` for sequencing, dependencies, and phase exit criteria
- `docs/internals/architecture.md` for tier and ownership constraints
- `docs/internals/guidelines.md` for support-tier and validation rules

## Phase Goal

Make the current `blusys::app` path truly canonical, internally coherent, and safe to build on.

Phase 0 must close the remaining foundation gaps before the platform expands further into widgets, themes, profiles, and archetype packaging.

## Phase Constraints

The following decisions are already locked by `PRD.md` and `ROADMAP.md` and must not be reopened during Phase 0:

- top-level runtime modes remain `interactive` and `headless`
- archetypes stay starter compositions, not framework branches
- product-facing code stays reducer-driven: `update(ctx, state, action)`
- product-facing namespace remains `blusys::app`
- product path is C++-only
- reducers mutate app state in place
- default onboarding is host-first interactive
- headless-first hardware is the secondary canonical path
- canonical interactive hardware profile remains generic SPI ST7735
- raw LVGL is allowed only inside custom widget implementations or explicit bounded custom view scope
- UI outward behavior goes through actions and approved framework behavior only
- hardware and capability configuration stays code-first
- the three-tier architecture stays intact

## Current Repo Baseline

The repo already has a real v7 engine:

- `blusys::app` exists and is structurally coherent
- the reducer model is real
- host, headless, and device entry points exist
- the ST7735 canonical device profile exists
- connectivity and storage capabilities already exist in early product-facing form
- a thin product-facing view layer exists

The main gaps are:

- routing and screen ownership are incomplete
- input and focus ownership are not fully integrated
- the theme system is too small for either vertical
- the stock widget set is too thin
- shared operational flows are mostly not built
- quickstarts, scaffold, docs, and examples are not yet fully canonicalized around the same path
- validation is still mostly build-centric rather than product-behavior-centric

## Phase Deliverables

Phase 0 should produce the following outputs:

1. Framework-owned route stack and screen ownership.
2. Unified input and focus handling across host and hardware paths.
3. Meaningful host and headless profile surfaces for product configuration.
4. Scaffold output aligned to the current app API.
5. Canonical fixed `core/`, `ui/`, and `integration/` app structure.
6. Canonical `main/` single-component scaffold model.
7. Quickstart, docs, and example cutover to `capabilities`.
8. A documented standard capability contract.
9. Removal of low-level framework examples from `examples/quickstart/`.
10. Scaffold dependency and pinning cleanup in `blusys create`.

## Workstreams

### Workstream A: Foundation Audit

Inventory the current product-facing surface before changing it.

Required outcomes:

- identify all product-facing entry points, quickstarts, scaffold templates, and docs references
- locate drift between `bundles` and `capabilities`
- locate drift between `app/` and `main/`
- identify routes, input bridges, and view APIs that are still exposed inconsistently

### Workstream B: Routing And Screen Ownership

Finish framework-owned route stack and screen registry behavior.

Required outcomes:

- route ownership moves into the framework path
- screen activation is explicit and framework-owned
- routing goes beyond overlay-only behavior
- screen lifecycle is consistent enough for the canonical app path

### Workstream C: Input And Focus Unification

Make the host and device interactive paths feel like one model.

Required outcomes:

- host and hardware input bridges use one coherent interaction grammar
- focus behavior is stable across host and device
- input ownership is not split across product code and framework code
- default interaction is predictable enough to trust during onboarding

### Workstream D: Host And Headless Profile Surfaces

Expose meaningful product-facing configuration surfaces for the two core runtime modes.

Required outcomes:

- host-first interactive profiles are obvious and runnable
- headless profiles remain first-class rather than a special case
- profile selection stays code-first and product-facing
- configuration is expressed through the recommended app path rather than plumbing

### Workstream E: Scaffold Cutover

Make generated projects match the canonical structure and build model.

Required outcomes:

- scaffolded apps use `main/` as the single local ESP-IDF component
- `main/CMakeLists.txt` compiles `core/`, `ui/`, and `integration/`
- scaffolded apps use the fixed `core/`, `ui/`, and `integration/` structure
- the separate local `app/` component model is removed from the canonical scaffold path
- `blusys create` dependency drift and pinning issues are fixed

### Workstream F: Naming Cutover And Capability Contract

Make `capabilities` the product-facing term and define the standard contract before more capabilities expand.

Required outcomes:

- public docs, quickstarts, scaffold output, and recommended APIs use `capabilities`
- long-lived parallel `bundle` naming is removed from the recommended path
- the standard capability contract is documented
- capability configuration, lifecycle, event mapping, and status rules are explicit

### Workstream G: Docs And Example Alignment

Align the public surface to the actual current API.

Required outcomes:

- quickstarts reflect the current `blusys::app` API
- docs resolve drift around entry macros, capability APIs, and view APIs
- low-level framework examples are removed from `examples/quickstart/`
- public guidance points to the canonical product path first

## Proposed Repository Landing Pattern

Phase 0 is mainly a cutover and alignment phase, so the landing pattern should emphasize the canonical product path rather than new breadth.

Recommended focus:

- keep the main interactive and headless quickstarts aligned with the current product API
- keep low-level framework material out of the quickstart path
- make `main/`, `core/`, `ui/`, and `integration/` the visible default for scaffolded apps

## Milestone Order

The recommended execution order for Phase 0 is:

1. Audit the current product-facing surface and identify drift.
2. Finish routing and screen ownership.
3. Unify input and focus handling.
4. Make host and headless profile surfaces meaningful.
5. Cut scaffold output over to the canonical structure and build model.
6. Cut over public terminology from `bundles` to `capabilities`.
7. Document the standard capability contract.
8. Align quickstarts, docs, and examples with the current API.
9. Remove low-level framework examples from `examples/quickstart/`.
10. Fix scaffold dependency drift and pinning issues in `blusys create`.

## Validation Plan

Phase 0 validation should prove canonicalization, not just compilation.

Required validation:

- `blusys host-build` for the canonical interactive quickstart
- targeted `blusys build` coverage for the ST7735 profile on `esp32`, `esp32c3`, and `esp32s3`
- targeted `blusys build` or equivalent headless validation for the canonical headless path
- scaffold smoke validation for generated starters
- docs build validation where navigation or public pages change
- inventory validation when examples or docs are added, removed, or reclassified

Recommended manual checklist:

- the recommended product path is obvious from the docs and scaffold output
- the host-first interactive starter runs without digging through low-level plumbing
- the headless starter stays minimal and does not require UI concepts
- the fixed project layout is visible in generated output
- the capability naming cutover is complete in the public surface

## Exit Criteria

Phase 0 is complete when all of the following are true:

- there is one unambiguous recommended product path
- quickstarts, scaffold, docs, and code all point to the same current API
- every scaffolded project uses the same fixed directory layout
- every scaffolded project uses `main/` as the single local component
- interactive routing is framework-owned, not mostly aspirational
- input behavior is stable enough to trust as a default path
- the public product-facing surface uses `capabilities` instead of `bundles`
- the standard capability contract is documented before broader capability expansion

## Risks To Watch

- leaving old and new product paths in parallel for too long
- fixing docs without fixing scaffold behavior, or vice versa
- allowing `integration/` to become a junk drawer for business logic
- keeping low-level framework examples too prominent in the product-facing path
- broadening widgets, themes, or profiles before the foundation is coherent

## Decision Rules During Phase 0

When implementation choices are ambiguous, prefer the option that:

- keeps the canonical product path smaller
- removes framework plumbing from normal app code
- makes host-first onboarding runnable by default
- preserves the three-tier architecture
- keeps raw LVGL and raw services visibly advanced

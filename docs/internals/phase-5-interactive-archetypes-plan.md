# Phase 5 Interactive Archetypes Plan

This document turns the Phase 5 target from `ROADMAP.md` into an execution-ready implementation plan for the current repo state.

Use it together with:

- `PRD.md` for product requirements and locked decisions
- `ROADMAP.md` for sequencing, dependencies, and phase exit criteria
- `docs/internals/architecture.md` for tier and ownership constraints
- `docs/internals/guidelines.md` for support-tier and validation rules

## Phase Goal

Prove that the existing `blusys::app` operating model can produce strong local interactive products without introducing a separate interaction-specific architecture.

Phase 5 must deliver two canonical interactive references:

- `interactive controller` as the primary reference implementation
- `interactive panel` as the secondary starter or reference

These references must prove:

- strong visual identity
- tactile-feeling interaction
- expressive transitions and feedback behavior
- compact and fluent navigation
- host-first iteration that stays close to device behavior
- no raw LVGL in normal app code

## Phase Constraints

The following decisions are already locked by `PRD.md` and must not be reopened during Phase 5:

- top-level runtime modes remain `interactive` and `headless`
- archetypes stay starter compositions, not framework branches
- product-facing code stays reducer-driven: `update(ctx, state, action)`
- product-facing namespace remains `blusys::app`
- canonical interactive hardware profile remains generic SPI ST7735
- UI outward behavior goes through actions and approved framework behavior only
- raw LVGL is allowed only inside custom widget implementations or explicit bounded custom view scope
- scaffold and product structure stay `main/`, `logic/`, `ui/`, and `system/`

## Current Repo Baseline

The current repo already contains the Phase 0 through Phase 4 foundation needed for archetype work:

- `blusys::app` runtime, reducer model, profiles, and entry macros are real
- theme presets already include `expressive_dark` and `operational_light`
- app-facing capabilities exist under `components/blusys_framework/include/blusys/app/capabilities/`
- shared flows exist under `components/blusys_framework/include/blusys/app/flows/`
- stock screens exist under `components/blusys_framework/include/blusys/app/screens/`
- shell support exists under `components/blusys_framework/include/blusys/app/view/shell.hpp`
- quickstarts already prove the generic interactive and connected paths

The main gap is not missing architecture. The main gap is missing productized interactive reference apps that pressure-test the current platform in a realistic way.

## Phase Deliverables

Phase 5 should produce the following outputs:

1. A primary `interactive controller` reference app.
2. A secondary `interactive panel` starter or reference app.
3. Framework refinements discovered from real archetype pressure.
4. Interaction and design guidance derived from the archetype work.
5. Validation coverage that proves host-first and ST7735-targeted interactive quality.

## Archetype Briefs

Before implementation work expands, both archetypes should be defined as concrete product starting points.

### Interactive Controller

This is the primary Phase 5 proof point.

- mode bias: interactive
- input bias: encoder first, buttons where appropriate, touch optional
- presentation bias: expressive, tactile, compact, high-character
- default theme direction: `expressive_dark`
- flow bias: settings, compact control flows, pairing or setup, status
- capability bias: storage first, connectivity optional, bluetooth optional

The controller reference should feel like a forkable product, not a widget gallery.

### Interactive Panel

This is the secondary Phase 5 proof point.

- mode bias: interactive
- input bias: touch, button array, or encoder depending on current bridge readiness
- presentation bias: operational, readable, slightly denser than controller
- default theme direction: `operational_light`
- flow bias: dashboard, settings, status, diagnostics, local control
- capability bias: connectivity and storage first, diagnostics optional

The panel reference should prove that the same shared platform can support a different interaction personality and information density without changing architecture.

## Workstreams

### Workstream A: Reference App Definition

Define a short implementation brief for each archetype before building deeper framework changes.

Each brief should lock:

- app purpose and primary user loop
- core screens
- required flows
- required capabilities
- input assumptions on host and device
- what makes the archetype feel productized

This keeps the reference apps from drifting into generic demos.

### Workstream B: Interactive Controller Reference

Build the primary controller reference first.

Recommended scope:

- one compact home or control screen
- one settings or configuration screen
- one status or about screen
- one setup, pairing, or provisioning entry point
- one feedback-rich control interaction path

Implementation rules:

- keep product behavior in `logic/`
- keep rendering and widgets in `ui/`
- keep capability setup and integration in `system/`
- use stock widgets and approved custom widgets only where necessary
- use framework shell and navigation ownership rather than custom routing
- use reducer actions for all user-visible behavior changes

This reference is the main pressure test for:

- compact navigation
- tactile feedback behavior
- host and device interaction parity
- expressive visual identity on top of shared framework pieces

### Workstream C: Interactive Panel Starter Or Reference

Build the panel artifact after the controller has exposed the first round of framework gaps.

Recommended scope:

- dashboard or status landing screen
- settings screen
- diagnostics or device-status entry point
- connectivity or provisioning visibility where appropriate
- local operator flow using the same shell, widgets, and app runtime model

This artifact should be intentionally less custom than the controller reference. Its job is to prove reuse and adaptability, not to maximize novelty.

### Workstream D: Framework Refinement From Product Pressure

As the archetypes are built, feed only the smallest broadly useful improvements back into the framework.

Expected refinement areas:

- token gaps around density, focus styling, motion, and status semantics
- widget gaps needed to avoid raw LVGL in normal archetype code
- stock screen and flow polish for settings, status, diagnostics, and setup
- shell and focus behavior polish needed for faster interactive navigation
- host and device parity improvements where interaction feel diverges too much

Rule: only generalize what clearly reduces product-owned burden across both archetypes or future forks.

### Workstream E: Docs, Examples, And Discoverability

When the references are credible, update the public surface so they become the main proof points for interactive products.

Required changes:

- add archetype guidance explaining when to start from controller vs panel
- classify the new archetype apps correctly in `inventory.yml`
- keep one clear canonical interactive quickstart path
- keep generic framework validation examples as reference material, not the main product story
- update app and internals docs when the archetypes reveal drift in naming or ownership guidance

## Proposed Repository Landing Pattern

The implementation should prefer a shape that mirrors the canonical product structure even inside examples.

Recommended naming:

- `examples/quickstart/interactive_controller/` or `examples/reference/interactive_controller/`
- `examples/reference/interactive_panel/` or `examples/quickstart/interactive_panel/`

Selection rule:

- use `quickstart` for the one archetype that becomes the main recommended interactive product entry
- use `reference` for the secondary archetype unless it also becomes part of the primary onboarding path

Each archetype app should still reinforce:

- `main/` as the single local component
- `logic/`, `ui/`, and `system/` ownership boundaries
- `blusys::app` as the only recommended product-facing API

## Milestone Order

The recommended execution order for Phase 5 is:

1. Write the controller and panel archetype briefs.
2. Build the `interactive controller` reference first.
3. Record framework gaps discovered during controller work.
4. Land the smallest reusable framework refinements.
5. Build the `interactive panel` starter or reference on the refined foundation.
6. Update docs, inventory, and example classification.
7. Add validation and visual smoke coverage.

This order keeps framework generalization driven by real product pressure instead of speculative abstractions.

## Validation Plan

Phase 5 validation must go beyond compile success.

Required validation:

- `blusys host-build` for both interactive archetypes
- targeted `blusys build` coverage for ST7735 on `esp32`, `esp32c3`, and `esp32s3`
- reducer tests for non-trivial app logic
- capability integration tests for flows that depend on runtime-service events
- screenshot or visual smoke coverage for main screens where practical
- manual host and hardware interaction checks for focus, navigation, feedback, and setup flows

Recommended manual checklist:

- focus traversal is obvious and stable
- confirm and cancel behavior is predictable
- transitions feel compact rather than ornamental
- feedback patterns feel distinct and intentional
- host interaction remains a credible stand-in for device iteration

## Exit Criteria

Phase 5 is complete when all of the following are true:

- the `interactive controller` reference feels tactile, expressive, and productized
- the `interactive panel` reference feels coherent and product-ready
- both references use the same shared reducer, runtime, shell, capability, and flow model
- neither reference requires raw LVGL in normal app code
- a product team could fork either reference and differentiate quickly without restructuring the app
- the docs and example surface clearly present these archetypes as real starting points rather than framework samples

## Risks To Watch

- overfitting framework APIs to one reference app
- allowing the controller reference to become a one-off visual demo
- allowing the panel reference to collapse into a generic dashboard sample
- broadening the widget or token surface before a repeated product need is clear
- keeping generic quickstarts more prominent than the archetype references after the phase is complete

## Decision Rules During Phase 5

When implementation choices are ambiguous, prefer the option that:

- keeps archetypes as compositions rather than branches
- removes product-owned burden rather than adding new framework vocabulary
- preserves host-first iteration quality
- improves both controller and panel reuse where possible
- keeps the change small and direct

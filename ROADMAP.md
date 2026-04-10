# V7 Refactor Roadmap

## Status

Active planning document. This roadmap tracks the phased execution of the v7 product-surface reset.

The companion product requirements document lives at `PRD.md`.

## Guiding Principle

This program is not about building a cleaner generic ESP32 framework.

This program is about building the internal operating system for our product families.

That means roadmap decisions should favor:

- recurring product archetypes over generic module sprawl
- shared product flows over app-by-app wiring
- strong default interaction and service behavior over many equal-looking choices
- expressive consumer products and reliable industrial products on the same core runtime

## Milestone Status

- [ ] Phase 0: Freeze the v7 contract
- [ ] Phase 1: Build `blusys::app` core
- [ ] Phase 2: Build the view layer and widget contract
- [ ] Phase 3: Build platform profiles
- [ ] Phase 4: Build service bundles
- [ ] Phase 5: Reset scaffold and CLI
- [ ] Phase 6: Shrink docs, examples, metadata, and CI
- [ ] Phase 7: Cut over to v7

## Reference Product Families

The roadmap should continuously validate the v7 platform against these recurring families:

- interactive consumer devices
- interactive control surfaces
- headless connected appliances and nodes
- industrial telemetry and control products

The first two canonical reference slices for implementation are:

- one host-first interactive product
- one headless connected product

## Workstreams

### Workstream A: App Core

Scope:

- `blusys::app` namespace
- reducer model
- product operating model
- runtime ownership
- navigation ownership
- effects model
- host, device, and headless entrypoints

### Workstream B: View Layer

Scope:

- stock widgets
- bindings
- design-system primitives
- page helpers
- overlays and navigation surfaces
- custom widget contract
- explicit custom LVGL view scope

### Workstream C: Platform And Services

Scope:

- host profile
- headless profile
- generic SPI ST7735 profile
- connected-device bundle
- storage bundle
- shared operational flows

Ownership note:

- service bundles and shared operational flows are framework-owned product-path behavior built on top of `blusys_services`, not new responsibilities for the services tier

### Workstream D: Tooling And Scaffolding

Scope:

- scaffold templates
- `blusys create`
- generated project shape
- first-run experience

### Workstream E: Repo Surface

Scope:

- docs structure
- example taxonomy
- reference-product coverage
- metadata inventory
- PR CI and nightly validation split

## Phase Plan

## Phase 0: Freeze the v7 contract

Goal:

Define the target before implementation so the refactor does not drift.

Deliverables:

- v7 PRD
- app model and reducer contract
- product family archetypes and reference slices
- shared operational flow inventory
- design-system and interaction grammar scope
- custom widget contract
- support-tier classification for modules, docs, and examples
- deprecation and removal list
- success metrics and release criteria

Exit Criteria:

- `PRD.md` exists
- the product contract is written down and stable enough to implement against
- the team can point to one source of truth for the refactor

## Phase 1: Build `blusys::app` core

Goal:

Create the new product-facing app runtime on top of the existing framework substrate.

Deliverables:

- `blusys::app` namespace skeleton
- `app_spec`
- `app_ctx`
- reducer-driven dispatch loop
- app-level operating model for both interactive and headless products
- internal navigation ownership
- internal feedback ownership
- entry macros for host, device, and headless
- default theme presets

Dependencies:

- Phase 0 complete

Exit Criteria:

- one interactive demo runs with no app-owned runtime plumbing
- one headless demo runs with the same reducer model
- normal app code no longer constructs low-level framework runtime objects directly
- both demos clearly map to the intended product families rather than generic examples

## Phase 2: Build the view layer and widget contract

Goal:

Make common product UI simple while preserving bounded LVGL freedom.

Deliverables:

- action-bound stock widgets
- simple bindings for text, value, enabled, and visible state
- reusable design-system primitives
- page and screen helpers
- built-in overlay and route helpers
- custom widget contract
- explicit custom LVGL view blocks

Dependencies:

- Phase 1 complete

Exit Criteria:

- canonical interactive app avoids raw widget handles for common flows
- custom product widgets have a formal extension pattern
- raw LVGL remains bounded to approved scopes
- the view layer is suitable for both expressive consumer UI and clear industrial UI

## Phase 3: Build platform profiles

Goal:

Make the new app model runnable on host, headless, and the first canonical interactive device path.

Deliverables:

- host default profile
- headless default profile
- generic SPI ST7735 profile
- framework-owned display and UI bring-up for the canonical path
- framework-owned common input bridge
- first product-family-aligned runtime slices on real targets

Dependencies:

- Phase 1 complete
- Phase 2 complete enough to support the canonical interactive example

Exit Criteria:

- host-first interactive quickstart runs immediately
- headless quickstart is first-class
- ST7735 profile builds on ESP32, ESP32-C3, and ESP32-S3
- at least one target has real hardware validation for the canonical ST7735 path
- the platform proves the same app model works for both interactive and headless families

Support note:

- milestone support for the ST7735 profile means compile support on all three targets
- release-ready support additionally requires staged real-hardware validation beyond the first validated target

## Phase 4: Build service bundles

Goal:

Remove low-level service orchestration from common product apps.

Deliverables:

- connected-device bundle over Wi-Fi with optional local control, mDNS, and SNTP
- unified storage bundle over the current storage surfaces
- clear advanced-only positioning for raw services in the docs and examples
- common diagnostics and control hooks for connected products

Dependencies:

- Phase 1 complete

Exit Criteria:

- canonical connected examples use bundles rather than raw service orchestration
- headless and interactive app paths can use services through app effects and bundles
- common connected product flows are shared across consumer and industrial examples

## Phase 5: Reset scaffold and CLI

Goal:

Make the new app model the first and most obvious user path.

Deliverables:

- scaffold templates extracted from the monolithic CLI script
- new generated project shape aligned with `blusys::app`
- runnable host-first interactive scaffold
- runnable headless scaffold
- simplified CLI surface for product creation
- scaffold outputs that map onto known product-family reference slices

Dependencies:

- Phases 1 through 3 complete enough to scaffold against

Exit Criteria:

- `blusys create` generates the new app model by default
- generated projects clearly separate framework glue from app-owned code
- getting-started can be rewritten around the new scaffold without caveats
- users can identify which quickstart matches their product family quickly

## Phase 6: Shrink docs, examples, metadata, and CI

Goal:

Reduce repo sprawl and validation cost once the new product path is real.

Deliverables:

- docs reorganized into `Start`, `App`, `Services`, `HAL + Drivers`, `Internals`, `Archive`
- examples reorganized into `quickstart`, `reference`, `validation`
- curated public example set
- reference products and family-oriented docs replacing generic sprawl where possible
- metadata-driven support and inventory checks
- PR CI narrowed to curated core builds and docs checks
- broader validation moved to nightly or release jobs

Dependencies:

- the new app path is stable enough to document and validate as canonical

Exit Criteria:

- public docs are scan-fast and consistent
- public example list is curated rather than exhaustive
- PR CI is materially smaller and faster than the current full example matrix
- docs and examples describe product families and common flows, not only module catalogs

## Phase 7: Cut over to v7

Goal:

Complete the reset and avoid carrying two product stories.

Deliverables:

- canonical quickstarts migrated to `blusys::app`
- old public app path archived or clearly demoted
- short `v6 -> v7` migration guide
- final cleanup of overlapping examples and stale docs
- the product-family operating-system mindset is reflected across the repo surface

Dependencies:

- prior phases complete

Exit Criteria:

- `blusys::app` is the only recommended product-facing API
- the old product path no longer appears as a peer path in onboarding materials

## First Milestone

The first implementation milestone combines the minimum work needed to make the reset real.

Scope:

- finish Phase 0
- complete the first cut of Phase 1
- produce one canonical interactive quickstart
- produce one canonical headless quickstart
- establish compile support for the generic SPI ST7735 profile on all three targets
- rewrite getting-started around the new path
- validate the milestone against the first two reference product families

Success Criteria:

- the new app model exists in code
- both canonical quickstarts run through the new reducer-driven framework path
- no scaffolded app-owned low-level runtime wiring remains in the new quickstarts
- the quickstarts already feel like the foundation of a product operating system, not just framework demos

## Tracking Checklist

### Product Strategy

- [ ] Define canonical product families
- [ ] Define first reference product slices
- [ ] Define shared interaction grammar
- [ ] Define shared operational flows
- [ ] Define design-system scope for v7

### App Core

- [ ] Define `app_spec`
- [ ] Define `app_ctx`
- [ ] Define reducer dispatch lifecycle
- [ ] Add host entry macro
- [ ] Add device entry macro
- [ ] Add headless entry macro
- [ ] Move route ownership inside the app runtime
- [ ] Move feedback ownership inside the app runtime

### View Layer

- [ ] Add action-bound button API
- [ ] Add simple value and text binding helpers
- [ ] Define reusable feedback primitives
- [ ] Add page helpers
- [ ] Add overlay helper path
- [ ] Define custom widget contract
- [ ] Define explicit custom LVGL scope

### Platform Profiles

- [ ] Add host default profile
- [ ] Add headless default profile
- [ ] Add generic SPI ST7735 profile
- [ ] Confirm ESP32 compile support
- [ ] Confirm ESP32-C3 compile support
- [ ] Confirm ESP32-S3 compile support
- [ ] Complete first hardware smoke on ST7735 profile

### Services

- [ ] Define connected-device bundle API
- [ ] Define storage bundle API
- [ ] Map bundles onto existing services
- [ ] Define diagnostics and local-control hooks for shared product flows

### Tooling

- [ ] Extract scaffold templates from `blusys`
- [ ] Create new interactive scaffold
- [ ] Create new headless scaffold
- [ ] Update CLI create flow

### Repo Cleanup

- [ ] Define new docs IA
- [ ] Define new example taxonomy
- [ ] Add metadata inventory source
- [ ] Add docs build gate to CI
- [ ] Reduce PR build matrix

## Success Metrics

- interactive scaffold runs on host immediately
- headless scaffold is similarly minimal
- canonical product code avoids `runtime.init`, `route_sink`, `feedback_sink`, `blusys_ui_lock`, and `lv_screen_load`
- public examples are reduced to a curated set
- PR CI becomes materially smaller and faster
- docs are fast to scan from `App` to `Services` to `HAL + Drivers` to `Internals`
- new products can map onto a known product family and shared flow model quickly

## Guardrails

- do not collapse the three-tier architecture
- do not add a heavy compatibility layer for the old product path
- do not allow direct service calls from UI code in the new model
- do not let raw LVGL escape outside approved custom scopes
- do not keep old and new public app models alive in parallel longer than necessary

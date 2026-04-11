# Roadmap

## Status

Active roadmap. This document defines the implementation sequence for the current product-platform reset.

The companion product requirements document lives at `PRD.md`.

**Program position:** Phase 4 (shared operational flows, stock screens, and framework capabilities) is **complete**. Phase 5 (interactive archetype references) is **complete**. Exit record: `docs/internals/phase-5-exit.md`. Next execution focus: **Phase 6** — connected archetype references.

## North Star

Blusys should become a unified internal product development platform, not a generic embedded framework.

It should let teams build two different vertical lenses on one shared operating model:

- **Consumer products** with tactile, expressive, characterful interaction and concise, fluent flows
- **Industrial and B2B products** with readable, dependable, operationally coherent flows and connected lifecycle management

What must stay shared underneath both:

- reducer-driven app model: `update(ctx, state, action)`
- app runtime and lifecycle ownership
- routing and navigation ownership
- capabilities and runtime-service orchestration model
- shared operational flows
- shared widget contract
- shared scaffold and host-first workflow

What should differ by product:

- visual language
- information density
- interaction emphasis
- reference archetype emphasis

## Archetype Model

The framework itself should only have two top-level runtime modes:

- `interactive`
- `headless`

Above that, the roadmap locks in four canonical archetypes.

These archetypes are not framework branches. They are medium-strength starter compositions for:

- docs
- examples
- scaffold defaults
- reference implementations

They are built from shared platform ingredients such as:

- capabilities
- profiles
- widgets
- themes
- flows
- input bridges

### Canonical Archetypes

Interaction-led archetypes:

- `interactive controller`
- `interactive panel`

Connected runtime archetypes:

- `edge node`
- `gateway/controller`

Rules:

- archetypes are optional starting shapes, not architecture boundaries
- archetypes may exist in `interactive` or `headless` variants when appropriate
- framework APIs should stay capability-based, not archetype-based
- archetypes are allowed to mix traits across consumer and industrial usage if the product needs it

### Starter Matrix

Each archetype needs a stable starter shape so scaffolds, docs, and examples do not drift.

- `interactive controller`
  - mode bias: interactive
  - default interaction: encoder, buttons, touch where appropriate
  - default capability bias: storage, connectivity optional, bluetooth optional
  - default flow bias: settings, pairing or setup, status, compact control flows
- `interactive panel`
  - mode bias: interactive
  - default interaction: touch, button array, encoder where appropriate
  - default capability bias: connectivity, storage, diagnostics optional
  - default flow bias: dashboard, settings, status, diagnostics, local control
- `edge node`
  - mode bias: headless-first
  - default interaction: no local UI by default, optional tiny status surface
  - default capability bias: connectivity, telemetry, storage, OTA, provisioning, diagnostics
  - default flow bias: provisioning, connectivity lifecycle, telemetry, diagnostics, maintenance
- `gateway/controller`
  - mode bias: headless or interactive depending on deployment
  - default interaction: optional local panel or operator surface
  - default capability bias: connectivity, telemetry, provisioning, diagnostics, OTA, local control
  - default flow bias: orchestration, diagnostics, settings, connectivity, maintenance, operator status

## Tier Model

The roadmap keeps the repository's three-tier architecture intact:

- `blusys` — HAL + drivers
- `blusys_services` — runtime substrate in C
- `blusys_framework` — product framework in C++

Interpretation:

- HAL + drivers own raw hardware-facing and device-near behavior
- the runtime substrate owns stateful runtime services such as Wi-Fi, BLE, OTA, MQTT, HTTP, UI runtime, filesystems, provisioning, and local control
- the framework owns reusable product-facing capabilities, flows, views, profiles, and app composition

This is important because Wi-Fi, Bluetooth, OTA, and similar modules are not just hardware drivers. They are stateful runtime systems and should not be collapsed into the HAL.

## Default Project Structure

Every scaffolded app should use the same default directory structure.

Archetypes should change the generated contents and defaults, not the project layout.

Recommended shape:

```text
my_project/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── core/
    ├── ui/
    └── integration/
```

Folder meanings:

- `core/` — product logic: `state`, `action`, `update(ctx, state, action)`, domain rules, app-owned behavior
- `ui/` — screens, custom widgets, theme or identity, view composition
- `integration/` — entrypoint wiring, selected profile, capability setup, runtime-service integration, mapping system events into app actions

Rules:

- this structure should be the same for interactive and headless projects
- `ui/` still exists for headless projects even if lightly used
- docs, examples, and scaffolds should all reinforce this same layout
- avoid per-archetype directory layouts that force developers to relearn project structure

### Directory Ownership Rules

To keep this structure intuitive and maintainable, the ownership boundaries must stay strict.

- `core/` owns product state, actions, reducer logic, and business behavior
- `core/` should not own profile selection, capability setup, or direct runtime-service wiring
- `ui/` owns screens, widgets, theme or identity, and rendering from state
- `ui/` should dispatch actions and read state, but should not call runtime services directly
- `ui/` should not contain business decisions that belong in the reducer
- `integration/` owns entrypoint assembly, profile selection, capability setup, bridges, and runtime-service integration
- `integration/` maps external events into app actions but should not become the home of product behavior
- `integration/` should not accumulate durable business state that belongs in `core/`

This separation is what makes the shared project structure scalable across all archetypes.

### Build Model Decision

The roadmap locks in one scaffold/build model for product apps:

- scaffolded product apps should use `main/` as the single local ESP-IDF component
- `main/CMakeLists.txt` should compile the files under `core/`, `ui/`, and `integration/`
- `main/idf_component.yml` remains the place where the platform components are pulled in through the managed component flow
- the current separate local `app/` component model should be removed from the recommended scaffold path during Phase 0
- in-repo examples may keep their own build mechanics where needed, but scaffolded product apps should no longer have two competing local layouts

This means the canonical scaffold shape is one local component with one internal folder structure, not a top-level `app/` component plus a different product layout inside it.

### Naming Cutover Decision

The roadmap also locks in the rename scope for `bundles` and `capabilities`:

- `capabilities` becomes the product-facing term in the framework
- the public recommended API, docs, scaffold output, examples, and quickstarts should use `capabilities`
- the rename should be executed as one deliberate Phase 0 cutover across the product-facing surface
- avoid keeping long-lived parallel `bundle` and `capability` names in the recommended path
- low-level runtime-service names inside `blusys_services` do not need to be rewritten just to match the new product language unless they surface directly in the product-facing API

This keeps the rename meaningful without turning it into unnecessary churn across unrelated internal layers.

### Capability Contract Decision

Every framework capability should follow one standard contract.

Required shape:

- each capability has a clear data-only configuration struct
- each capability is composed in `integration/`, not in `core/` or `ui/`
- framework or integration wiring owns capability lifecycle and runtime-service integration
- capability status is queryable through a consistent app-facing mechanism
- product logic requests capability work through reducer-driven actions; `integration/` translates those actions into capability calls
- capability events are mapped into app actions in `integration/`
- `core/` reacts to those actions in the reducer and owns product behavior
- `ui/` renders from state and dispatches actions, but does not call runtime services directly
- capabilities may expose advanced escape hatches, but the recommended path stays reducer- and event-driven

This contract is mandatory before multiple capabilities are expanded in parallel.

## Current-State Reality

The repo already has a real v7 engine:

- `blusys::app` exists and is structurally coherent
- the reducer model is real
- host, headless, and device entry points exist
- the ST7735 canonical device profile exists
- connectivity and storage capabilities already exist in early product-facing form
- a thin product-facing view layer exists

But the platform is not yet the finished product operating system we want.

The most important current gaps are:

- routing and screen ownership are incomplete
- input and focus ownership are not fully integrated
- the theme system is too small for either vertical
- the stock widget set is too thin
- shared operational flows are mostly not built
- quickstarts, scaffold, docs, and examples are not yet fully canonicalized around the same path
- validation is still mostly build-centric rather than product-behavior-centric

This means the right sequencing is:

1. close the remaining foundation gaps
2. build the shared product operating system layer
3. prove it through the canonical archetypes without turning them into framework branches
4. package the proven model into broader profiles, scaffolds, and docs

## Product Principles

- push complexity down into the platform
- keep product code centered on `state`, `action`, `update(ctx, state, action)`, views, capabilities, and profiles
- keep the three-tier architecture intact
- build shared foundations first, vertical specialization second
- keep raw LVGL and raw services available, but visibly advanced
- validate through archetype references, not only through abstractions and docs
- keep one fixed app directory structure across all scaffolded projects
- keep the runtime substrate in C; do not pull stateful runtime services down into the HAL
- use one standard capability contract so all product-facing capabilities compose the same way

## Phase Plan

## Phase 0: Close The V7 Foundation

### Goal

Make the current `blusys::app` path truly canonical, internally coherent, and safe to build on.

### Why this phase is mandatory first

The current engine is real, but repo-level coherence still lags behind it. Expanding widgets, themes, and profiles before closing those gaps would increase breadth without increasing trust.

### Deliverables

- finish real framework-owned route stack and screen ownership
- move beyond overlay-only routing behavior
- unify input and focus handling across host and hardware paths
- make host and headless profiles meaningful product-facing configuration surfaces
- align scaffold output, quickstarts, docs, and examples with the actual current app API
- establish and enforce the fixed `core/`, `ui/`, and `integration/` app structure for every scaffolded project
- remove the separate local `app/` component model from the canonical scaffolded product path and standardize on `main/` as the single local component
- remove low-level framework examples from `examples/quickstart/` so quickstarts are purely product-facing
- fix scaffold dependency drift and pinning issues in `blusys create`
- resolve obvious docs drift around entry macros, capability APIs, and view APIs
- cut over the product-facing API, docs, and scaffold language from `bundles` to `capabilities`
- define and document the standard capability contract before expanding the capability catalog

### Exit Criteria

- there is one unambiguous recommended product path
- quickstarts, scaffold, docs, and code all point to the same current API
- every scaffolded project uses the same fixed directory layout
- every scaffolded project uses `main/` as the single local component
- interactive routing is framework-owned, not mostly aspirational
- input behavior is stable enough to trust as a default path

## Phase 1: Design System And Product Identity Foundation

### Goal

Create one shared token and identity system that can express both verticals without splitting the platform.

### Deliverables

- expand `theme_tokens` with:
  - semantic colors
  - state colors
  - density controls
  - richer typography scale
  - motion tokens
  - depth or elevation tokens
- add a compact icon system suitable for small displays and dense status surfaces
- define product identity hooks for theme, motion, feedback, and icon usage
- ship at least two serious presets:
  - `expressive_dark`
  - `operational_light`
- define framework-owned feedback presets for:
  - click
  - confirm
  - success
  - warning
  - error
  - notification

### Exit Criteria

- the same widget code can look like a tactile interactive controller or an operational interactive panel just by changing identity and theme inputs
- motion, density, and feedback are token-driven rather than ad hoc
- widgets no longer assume one narrow visual language

## Phase 2: Interaction Shell And Navigation Ownership

### Goal

Make the framework own the common interaction grammar for interactive products.

### Deliverables

- real route stack and screen registry behavior
- route transitions driven by motion tokens
- persistent header and status surfaces
- tab navigation support
- stronger page lifecycle ownership
- unified encoder flow across host and device
- hardware button-array input bridge
- touch input bridge
- a consistent focus model for interactive widgets and screens

### Exit Criteria

- interactive products no longer hand-roll navigation structure
- focus and input behavior feel consistent across host and device
- both verticals can use the same shell while presenting different personalities and densities

## Phase 3: Shared Widget Library Expansion

### Goal

Cover the majority of normal product UI needs without dropping to raw LVGL.

### Deliverables

Shared core widgets:

- `progress`
- `list`
- `card`
- `icon_label`
- `status_badge`
- `input_field`
- `tabs`
- `dropdown` or picker

Consumer-emphasis widgets:

- `gauge`
- `knob`
- audio-control friendly display widgets

Industrial-emphasis widgets:

- `data_table`
- `chart`
- `key_value`

All widgets must:

- follow the six-rule widget contract
- consume theme tokens only
- preserve reducer-driven outward behavior
- support the standard focus and disabled model where interactive

### Exit Criteria

- most normal app UI can stay inside `blusys::app`
- interactive controller and panel screens can be built from stock widgets
- edge-node and gateway local surfaces can be built from stock widgets where needed

## Phase 4: Shared Operational Flows And Stock Product Screens

### Goal

Turn the platform into a reusable product operating system instead of only a runtime engine.

### Deliverables

Shared flows:

- boot and splash
- loading and empty states
- error and recovery states
- settings and configuration
- provisioning and setup
- connectivity state and reconnect UX
- diagnostics and local control entry points
- OTA and maintenance flow
- status, alerts, and confirmations

Framework-owned stock screens and builders:

- settings screen builder
- about or device-info screen
- status screen
- diagnostics screen
- provisioning flow screens for interactive products
- headless equivalents through capability events and reducer integration

Framework-owned capabilities on top of the runtime substrate:

- `connectivity_capability` — Wi-Fi, SNTP, mDNS, local control, and reconnect behavior
- `bluetooth_capability` — BLE-oriented setup and device communication paths where applicable
- `storage_capability` — persistent local data management
- `ota_capability` — firmware update lifecycle
- `diagnostics_capability` — health, system info, and diagnostic surfaces
- `telemetry_capability` — metric collection, buffering, and delivery
- `provisioning_capability` — first-run setup and reprovisioning flows

Capability contract requirements:

- capability configuration lives in `integration/`
- capability lifecycle is owned by framework or system wiring, not by screens
- product logic requests capability work through reducer-driven actions; `integration/` performs the capability call
- capability status and events cross into product code only through the standard app-facing contract
- business decisions stay in `core/`
- view behavior stays in `ui/`

### Exit Criteria

- products stop rebuilding system flows app by app
- both consumer and industrial products use the same flow ownership model
- product code customizes behavior through reducer actions and events rather than forking platform flows

### Status

**Closed** — Phase 4 deliverables are in the tree. Cross-cutting validation (tests, CI breadth, host iteration) continues on the [Validation And Developer Loop](#cross-cutting-workstream-validation-and-developer-loop) workstream, not as a Phase 4 gate.

## Phase 5: Interactive Archetype References

Detailed execution plan: `docs/internals/phase-5-interactive-archetypes-plan.md`

### Status

**Closed** — 2026-04-11. See `docs/internals/phase-5-exit.md`.

### Goal

Prove the platform can build strong local interactive products without adding a separate interaction-specific architecture.

### Canonical Interactive Archetypes

- `interactive controller`
- `interactive panel`

### What these references must prove

- strong visual identity
- tactile-feeling interaction
- expressive transitions and feedback behavior
- compact and fluent navigation
- host-first iteration that feels close to the device experience
- no raw LVGL in normal app code

### Deliverables

- a primary reference implementation for `interactive controller`
- a secondary archetype starter or reference for `interactive panel`
- interaction and design guidance derived from the actual archetype work
- refinement of tokens, icons, widgets, and feedback discovered from real product pressure

### Exit Criteria

- the interactive archetypes feel like real product starting points, not framework samples
- a product team can fork either archetype and quickly produce a differentiated product

## Phase 6: Connected Archetype References

Detailed execution plan: `docs/internals/phase-6-connected-archetypes-plan.md`

### Goal

Prove the platform can build dependable connected products without adding a separate connectivity-specific architecture.

### Canonical Connected Archetypes

- `edge node`
- `gateway/controller`

### What these references must prove

- clear operational state at a glance
- reliable connectivity behavior and recovery
- diagnostics and operational status visibility
- OTA, telemetry, and local control readiness
- headless-first viability with an optional local UI surface where appropriate
- no repeated low-level orchestration in app code

### Deliverables

- a primary reference implementation for `edge node`
- a secondary archetype starter or reference for `gateway/controller`
- connected-product guidance derived from the actual archetype work
- telemetry capability maturity driven by real product needs
- proof that the same operating model supports both headless deployment paths and interactive local-surface variants where useful

### Exit Criteria

- the connected archetypes feel operationally complete, not like technical demos
- a product team can fork either archetype and quickly produce a differentiated product

## Phase 7: Hardware Breadth And Platform Packaging

### Goal

Support the hardware and packaging breadth that the proven product model actually needs.

### Deliverables

- ST7789 profile
- SSD1306 or SH1106 profile
- ILI9341 profile
- ILI9488 profile
- layout helpers for density, rotation, and resolution adaptation
- archetype-aligned packaging of profiles, widgets, and flows for the four canonical starting shapes

### Exit Criteria

- the reference products run across more than one profile without app rewrites
- adding a new profile does not require new product architecture
- display and input breadth follows validated product pressure rather than speculative catalog expansion

## Phase 8: Ecosystem, Archetype Scaffolds, And Docs

### Goal

Make the proven platform discoverable directly from `blusys create` and easy to adopt without repo archaeology.

### Deliverables

- starter compositions for the four canonical archetypes
- archetype starters that all keep the same `core/`, `ui/`, and `integration/` project structure
- archetype starters that all use `main/` as the single local component
- docs paths that explain the four archetypes clearly while still preserving consumer and industrial design guidance on top of the shared foundation
- widget gallery and capability composition guides
- quickstart and scaffold flows centered on known archetypes instead of blank technical shells

### Exit Criteria

- a new developer can identify the right product family quickly
- a new product starts from an archetype, not from a blank technical shell
- the archetype choice changes defaults and starter content, not the project layout
- the public repo surface reflects the platform mindset consistently

## Cross-Cutting Workstream: Validation And Developer Loop

This should not wait until the end. It should progress alongside the core phases.

Required validation direction:

- host-runnable reducer tests for app logic
- capability integration tests with simulated runtime-service events
- screenshot or visual smoke coverage for shared screens and reference products where practical
- scaffold smoke validation for generated starters
- curated CI coverage aligned with the canonical product path

Required developer-loop direction:

- keep host-first iteration fast and central
- improve quickstart clarity before adding more breadth
- prefer small, direct tooling improvements over a premature large CLI rewrite

## Workstream View

### Workstream A: Foundation Closure

- canonical app path completion
- routing completion
- input and focus completion
- docs, scaffold, and example alignment

### Workstream B: Design System And Identity

- token expansion
- presets
- motion and feedback semantics
- icon and typography system

### Workstream C: Interaction And Widgets

- navigation shell
- input bridges
- stock widget expansion

### Workstream D: Product Operating Flows

- settings
- provisioning
- diagnostics
- OTA
- status and alerts
- reusable runtime capabilities

### Workstream E: Archetype Proof

- interactive controller archetype
- interactive panel archetype
- edge node archetype
- gateway/controller archetype

### Workstream F: Validation And Developer Loop

- reducer tests
- capability tests
- visual smoke
- scaffold validation
- host iteration quality

### Workstream G: Hardware And Ecosystem

- additional profiles
- archetype-based scaffolds
- docs and gallery system

## Dependency Order

The practical dependency order should be:

```text
Phase 0 -> Phase 1 -> Phase 2 -> Phase 3 -> Phase 4
                               |         |
                               |         +-> Phase 5
                               |         +-> Phase 6
                               |
                               +---------------> Phase 7

Phase 5 + Phase 6 -> Phase 8
```

Interpretation:

- Phase 0 is mandatory first
- Phase 1 unlocks meaningful vertical expression
- Phases 2 and 3 can overlap carefully
- Phase 4 should land before the reference products are considered complete
- Phase 7 should follow proven reference-product pressure, not lead it
- Phase 8 should package what has already been validated by real product references

## Success Metrics

### Platform Metrics

- one shared reducer-based runtime powers interactive and headless products across consumer and industrial use cases
- most product code stays inside `blusys::app`
- common flows are framework-owned rather than rebuilt per app
- widget coverage handles most product needs without raw LVGL

### Interactive Metrics

- the interactive controller archetype feels tactile, expressive, and productized
- the interactive panel archetype feels coherent and product-ready
- the host path supports rapid iteration without losing interaction fidelity
- product teams can fork the archetypes and differentiate quickly

### Connected Metrics

- the edge node archetype feels dependable, connected, and operationally clear
- the gateway/controller archetype feels reliable and operationally coherent
- diagnostics, telemetry, OTA, and status flows are reusable and coherent
- the same product model supports headless and local-surface variants where needed

### Repo Metrics

- quickstarts are truly canonical and product-facing
- docs are current and scan-fast
- scaffold output matches the recommended path exactly
- every generated project uses the same fixed directory structure
- every generated project uses the same canonical build model
- CI validates the curated, intentional product surface rather than a confused mixture of old and new stories

## Guardrails

- do not collapse the three-tier architecture
- do not redesign HAL or services unless a concrete blocker requires it
- keep capabilities and operating flows in `blusys::app`, built on top of the runtime substrate
- do not allow raw LVGL to become the normal path again
- do not let UI code call services directly in the default model
- build shared foundations first, vertical specialization second
- require the archetype references to be proven before declaring the platform finished
- require the four canonical archetypes to be reflected in docs, examples, and starters before declaring the platform stable

## Out Of Scope For This Core Roadmap

These are not forbidden forever, but they should not define the core platform program right now:

- migrating the services tier to C++ or adding broad C++ service facades as a mainline dependency
- major target expansion beyond `esp32`, `esp32c3`, and `esp32s3`
- a ground-up CLI rewrite before the canonical product path is fully stable
- cloud-platform breadth that outruns the shared product model

## Final End-State

The platform is successful when:

- consumer products and industrial products are built on the same operating model
- the framework stays centered on `interactive` and `headless`, while archetypes remain optional starter compositions above that layer
- consumer products can feel fluent, tactile, and memorable without repeated low-level reinvention
- industrial products can feel clear, dependable, and connected without repeated orchestration work
- new teams start from shared product grammar, not from plumbing
- Blusys behaves like an internal product operating system rather than a loose collection of modules

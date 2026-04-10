# Blusys Product Platform PRD

## Status

Active product requirements document. This file defines the target product shape for the current platform reset and is the source of truth for scope, goals, and constraints.

The companion implementation plan lives at `ROADMAP.md`.

## Problem Statement

Blusys already has a coherent internal three-tier architecture, but too much runtime wiring, UI setup, lifecycle management, and low-level orchestration still leaks into product code. The result is:

- application code is more verbose than it should be
- interactive products are not truly runnable by default
- headless products still require too much manual service orchestration
- product logic is too tightly coupled to framework internals and LVGL details
- the public learning surface is too wide across examples, docs, and setup flows
- the repo is harder to maintain than necessary

The platform needs a breaking reset that makes product code dramatically simpler, pushes complexity into the framework, and preserves low-level power as an advanced path rather than the default path.

## Platform Framing

Blusys is not being built as a generic ESP32 framework.

Blusys is being built as the internal operating system for our recurring product shapes.

That means the platform should optimize for the products we actually build and ship, not for maximum generic breadth. The goal is to give every new product a shared operating model, shared lifecycle, shared interaction grammar, and shared runtime-service orchestration, while still allowing each product to express its own identity.

## Vision

Blusys should let a product team build a working MVP quickly with a minimal amount of product-owned code.

The product-facing path should be opinionated, small, and easy to scan:

- define app state
- define actions
- implement `update(ctx, state, action)`
- define screens and custom widgets if needed
- select optional profiles and capabilities
- run the same app on host, device, or headless targets

Everything else should be framework-owned unless there is a strong reason to expose it.

The target outcome is a platform that gives us:

- consumer products with tactile, expressive, high-character interaction and presentation
- industrial and business products with clear, reliable, operationally coherent flows
- one shared internal product operating model underneath both

Consumer inspiration is closer to Teenage Engineering: distinctive, tactile, concise, memorable.

Industrial inspiration is closer to Samsara: readable, dependable, operationally fluent, connected.

## Users

Primary users:

- product developers building MVPs and production apps on ESP32 targets
- internal maintainers evolving the product framework and product path

Secondary users:

- advanced embedded users who need direct HAL, runtime-service, or LVGL access

## Product Goals

1. Make the default product path much smaller and simpler.
2. Make host-first interactive prototyping the default onboarding path.
3. Make headless-first hardware a first-class secondary path.
4. Build one shared operating model across our recurring product shapes.
5. Provide reusable interaction, connectivity, diagnostics, provisioning, storage, and maintenance flows across products.
6. Keep the low-level layers available, but demote them from the recommended product workflow.
7. Make the repo easier to maintain by shrinking and curating the public surface.

## Non-Goals

- collapsing the three-tier architecture
- migrating HAL or runtime services to C++ as part of this reset
- preserving backwards compatibility with the previous product-facing flow
- building a large reactive UI engine or virtual DOM
- forcing user widgets into a heavy inheritance hierarchy
- introducing B2B and B2C as hard framework modes

## Locked Decisions

- The product-facing namespace is `blusys::app`.
- The product path is C++-only.
- App logic follows a reducer model: `update(ctx, state, action)`.
- Reducers mutate app state in place.
- Default onboarding is host-first interactive.
- Headless-first hardware is the secondary canonical path.
- The only core runtime modes are `interactive` and `headless`.
- Consumer and industrial are product lenses, not framework branches.
- The first canonical interactive hardware profile is a generic SPI ST7735 profile.
- The ST7735 profile must compile on ESP32, ESP32-C3, and ESP32-S3 from the first milestone; release readiness additionally requires real hardware validation on at least one target and staged validation across the others.
- Raw LVGL is allowed only inside custom widget implementations or explicit custom view scope.
- UI code must communicate outward only through app actions and approved framework behavior.
- Hardware and capability configuration is code-first, with Kconfig reserved for advanced tuning.
- HAL, runtime services, `blusys_ui`, and low-level framework primitives remain as advanced escape hatches.

## Modes And Archetypes

### Core Runtime Modes

The framework itself should expose only two top-level runtime modes:

- `interactive`
- `headless`

These are fundamental runtime forms, not product categories.

### Canonical Archetypes

Above the two runtime modes, the platform should support four canonical archetypes as starter compositions:

- `interactive controller`
- `interactive panel`
- `edge node`
- `gateway/controller`

These archetypes are:

- starter compositions for docs, examples, and scaffolds
- medium-strength defaults rather than hard architecture branches
- built from shared capabilities, profiles, widgets, themes, flows, and input bridges

Archetypes may exist in `interactive` or `headless` variants where appropriate.

Starter matrix:

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

The repository keeps its three-tier architecture:

- `blusys` — HAL + drivers
- `blusys_services` — runtime substrate in C
- `blusys_framework` — product framework in C++

Interpretation:

- HAL + drivers own raw hardware-facing and device-near behavior
- the runtime substrate owns stateful runtime services such as Wi-Fi, BLE, OTA, MQTT, HTTP, UI runtime, filesystems, provisioning, and local control
- the framework owns reusable product-facing capabilities, flows, views, profiles, and app composition

Wi-Fi, Bluetooth, OTA, and similar modules are not just hardware drivers. They are stateful runtime systems and should not be collapsed into the HAL.

## Default Project Structure

Every scaffolded product app should use the same default project structure.

Recommended shape:

```text
my_project/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── logic/
    ├── ui/
    └── system/
```

Folder meanings:

- `logic/` — product state, actions, reducer logic, and business behavior
- `ui/` — screens, widgets, theme or identity, and rendering from state
- `system/` — entrypoint wiring, selected profile, capability setup, runtime-service integration, and mapping system events into app actions

Directory ownership rules:

- `logic/` owns product behavior and should not own direct runtime-service wiring
- `ui/` renders from state and dispatches actions, but should not call runtime services directly
- `system/` owns setup, wiring, bridges, and runtime integration, but should not become the home of product behavior

### Build Model

Scaffolded product apps should use `main/` as the single local ESP-IDF component.

- `main/CMakeLists.txt` compiles the files under `logic/`, `ui/`, and `system/`
- `main/idf_component.yml` remains the place where platform components are pulled in through the managed component flow
- the separate local `app/` component model is not part of the canonical scaffold path

## Capability Model

The product-facing framework should use `capabilities` as the canonical term.

This is a product-language decision:

- public docs, quickstarts, scaffold output, and recommended APIs should use `capabilities`
- low-level runtime-service names inside `blusys_services` do not need to be rewritten unless they surface directly in the product-facing API

The framework should expose reusable product-facing capabilities such as:

- connectivity
- bluetooth
- storage
- OTA
- diagnostics
- telemetry
- provisioning

### Capability Contract

Every framework capability must follow one standard contract.

Required shape:

- each capability has a clear data-only configuration struct
- each capability is composed in `system/`, not in `logic/` or `ui/`
- framework or system wiring owns capability lifecycle and runtime-service integration
- capability status is queryable through a consistent app-facing mechanism
- product logic requests capability work through reducer-driven actions; `system/` translates those actions into capability calls
- capability events are mapped into app actions in `system/`
- `logic/` reacts to those actions in the reducer and owns product behavior
- `ui/` renders from state and dispatches actions, but does not call runtime services directly
- capabilities may expose advanced escape hatches, but the recommended path stays reducer- and event-driven

## Product-Owned Versus Framework-Owned Responsibilities

The product path should center on these app-owned concepts:

- `state`
- `action`
- `update(ctx, state, action)`
- screens and views
- custom widgets where needed
- optional profiles
- optional capabilities

The framework should own these responsibilities:

- application boot and shutdown
- runtime loop and tick cadence
- navigation and route ownership
- feedback defaults and feedback plumbing
- LVGL lifecycle and lock discipline
- screen activation and overlay handling
- host, device, and headless adapters
- common input bridges
- common runtime-service orchestration for the default path
- reusable operating flows for common product behaviors

## Product Operating System Requirements

The platform should provide one shared internal operating model across product shapes.

That operating model should standardize:

- state and action flow
- navigation and screen ownership
- input semantics
- feedback semantics
- settings and configuration patterns
- connectivity lifecycle and recovery
- diagnostics and local control entry points
- update and maintenance flow hooks
- product status reporting and alert surfaces

The goal is for new products to start from a shared product grammar instead of re-inventing runtime behavior each time.

## Functional Requirements

### 1. App Runtime

The framework must provide a high-level app runtime that hides low-level framework plumbing from normal product code.

Required capabilities:

- define an app through a single `app_spec`
- run that app on host, device, or headless targets
- own the route stack and feedback sinks internally
- provide an `app_ctx` for dispatch, navigation, effects, and approved framework capabilities
- keep lower-level runtime primitives available as advanced or internal APIs

### 2. Interactive And Headless Paths

The interactive and headless paths must use the same reducer and app model, with UI removed rather than a separate architecture.

Required capabilities:

- host-first quickstart that runs immediately
- framework-owned display and UI boot for the canonical interactive path
- built-in navigation and overlay handling
- same runtime core for the headless path
- no UI or LVGL dependencies in the normal headless product path

### 3. Design System And Interaction Grammar

The framework must ship reusable design and interaction primitives that support both consumer and industrial products.

Required capabilities:

- theme tokens for color, spacing, type, density, and motion
- default presets for expressive and operational presentation styles such as `expressive_dark` and `operational_light`
- standard focus, input, and confirmation behavior
- reusable notification, overlay, and transient feedback patterns
- reusable settings, status, and control-surface patterns
- coherent feedback behavior across visual, audio, and haptic channels

### 4. View System And Widget Contract

The framework must provide a simple, opinionated view layer that keeps common apps off raw LVGL while still permitting bounded customization.

Required capabilities:

- stock action-bound widgets for common controls
- simple bindings for text, value, enabled, and visible state
- page and layout helpers for common screens
- custom widget support through a formal contract rather than inheritance
- explicit custom LVGL blocks for advanced rendering inside a defined scope

Custom widget rules:

- public `config` or `props` struct as the widget interface
- raw LVGL remains inside the widget implementation
- visuals come from theme tokens only
- setters own widget state transitions when runtime mutation is needed
- widget behavior emits semantic callbacks or dispatches app actions
- interactive widgets support the standard focus and disabled model

### 5. Framework Capabilities

The framework must provide higher-level capabilities for common product needs so apps stop assembling low-level runtime-service lifecycles manually.

Ownership rule: these capabilities belong in the framework-owned `blusys::app` path, composed on top of `blusys_services`. They must not shift orchestration responsibility back into product apps or sideways into the runtime substrate.

Required initial capabilities:

- connectivity capability over Wi-Fi with optional local control, mDNS, and SNTP
- storage capability over the current storage surfaces
- bluetooth capability for BLE-oriented setup and device communication where appropriate
- OTA capability
- diagnostics capability
- telemetry capability
- provisioning capability

Raw runtime-service APIs remain available but are not the recommended first path.

### 6. Shared Operational Flows

The framework must progressively ship reusable operating flows that reduce repeated app work across product shapes.

Ownership rule: these flows are framework-owned product behaviors built on top of the runtime substrate and HAL, not new runtime-service-tier responsibilities.

Required flow targets include:

- boot and startup state
- loading and empty states
- error and recovery states
- settings and configuration
- provisioning and initial setup
- connectivity state and reconnect behavior
- local control and diagnostics
- OTA and maintenance flow
- status, alerts, and confirmation feedback

### 7. Scaffold And CLI

The scaffold and CLI must reflect the new product model.

Required capabilities:

- `blusys create` generates a runnable host-first interactive product by default
- headless scaffold is equally first-class
- generated glue is clearly separated from user-owned product logic
- generated projects use the fixed `logic/`, `ui/`, and `system/` structure
- scaffold templates live as real files, not inline heredocs inside a large script
- the first-run path uses the new app model only

### 8. Docs And Discoverability

Docs must be fast to scan from app surface to lower layers.

Required top-level flow:

- `Start`
- `App`
- `Services`
- `HAL + Drivers`
- `Internals`
- `Archive`

Docs should emphasize the recommended product path first and keep advanced surfaces clearly marked.

### 9. Validation And Developer Loop

The product path needs proportional validation and a strong host-first loop.

Required capabilities:

- reducer tests runnable on host
- capability integration tests with simulated runtime-service events
- screenshot or visual smoke coverage where practical
- scaffold smoke validation for generated starters
- curated CI coverage aligned with the canonical product path
- fast host iteration without flashing hardware for every change

## User Experience Requirements

### Quickstart Experience

A user should be able to:

- create a new product
- run it on host immediately for the interactive path
- understand where product code lives and what to edit first
- find the headless quickstart without navigating through low-level examples first
- recognize which archetype is the best starting point

### Product Code Simplicity

Normal product code should not need to use these concepts directly:

- `runtime.init`
- `route_sink`
- `feedback_sink`
- `blusys_ui_lock`
- `lv_screen_load`
- raw LCD bring-up for the canonical path
- raw Wi-Fi connect orchestration for the canonical path

## Technical Principles

1. Push complexity downward into the framework.
2. Build for our recurring product shapes, not generic embedded breadth.
3. Prefer one strong default path over multiple equal-looking paths.
4. Keep low-level escape hatches available but visibly advanced.
5. Keep the implementation small and direct.
6. Avoid adding new abstractions unless they remove real product-code burden.
7. Keep one fixed scaffold structure across product apps.
8. Keep runtime services in C and product-facing composition in the framework.

## Success Metrics

Product code metrics:

- scaffolded interactive product runs on host immediately
- scaffolded headless product is similarly minimal
- canonical product apps avoid low-level framework and UI lifecycle plumbing
- new products can map cleanly onto a known archetype and shared flow model

Repo and maintenance metrics:

- public examples are reduced to a curated set
- PR CI is materially smaller and faster
- docs are faster to scan and more consistent
- duplicate or stale public guidance is removed
- scaffold output matches the canonical structure and build model exactly

Developer experience metrics:

- time from `blusys create` to first running host app is short and obvious
- users can find the right app-level guide before they need to read low-level module docs
- teams can reuse shared product flows and capabilities instead of rebuilding them per product

## Risks

- over-designing the new app layer instead of keeping it minimal
- building a cleaner framework that is still too generic for our real product work
- allowing raw LVGL or raw runtime services to leak back into the default path
- allowing `system/` to become a junk drawer for business logic
- carrying old and new product paths in parallel for too long
- delaying the scaffold and docs cutover until after too many lower-level changes pile up

## Out Of Scope For The Core Cut

- broad runtime-service redesigns beyond the initial capability surface
- large-scale HAL API redesign
- advanced board-profile catalog beyond the first canonical interactive profile
- major target support expansion beyond `esp32`, `esp32c3`, and `esp32s3`
- a full CLI rewrite before the canonical product path is stable

## Release Criteria

The reset is ready for cutover when:

- `blusys::app` is the only recommended product-facing API
- interactive and headless quickstarts both use the new app model
- the generic SPI ST7735 profile exists and builds on all three targets
- the scaffold and getting-started flow use the new model
- the default scaffold uses `main/` as the single local component and the fixed `logic/`, `ui/`, `system/` structure
- the product-facing language has cut over from `bundles` to `capabilities`
- the platform clearly supports the four canonical archetypes through shared app and operating flows

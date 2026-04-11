# Phase 6 Connected Archetypes Plan

This document turns the Phase 6 target from `ROADMAP.md` into an execution-ready implementation plan for the current repo state.

Use it together with:

- `PRD.md` for product requirements and locked decisions
- `ROADMAP.md` for sequencing, dependencies, and phase exit criteria
- `docs/internals/architecture.md` for tier and ownership constraints
- `docs/internals/guidelines.md` for support-tier and validation rules
- `docs/internals/phase-5-interactive-archetypes-plan.md` for the companion interactive archetype plan

## Phase Goal

Prove that the existing `blusys::app` operating model can produce dependable connected products without introducing a separate connectivity-specific architecture.

Phase 6 must deliver two canonical connected references:

- `edge node` as the primary reference implementation
- `gateway/controller` as the secondary starter or reference

These references must prove:

- clear operational state at a glance
- reliable connectivity behavior and recovery
- diagnostics and operational status visibility
- OTA, telemetry, and local control readiness
- headless-first viability with an optional local UI surface where appropriate
- no repeated low-level orchestration in app code

## Phase Constraints

The following decisions are already locked by `PRD.md` and must not be reopened during Phase 6:

- top-level runtime modes remain `interactive` and `headless`
- archetypes stay starter compositions, not framework branches
- product-facing code stays reducer-driven: `update(ctx, state, action)`
- product-facing namespace remains `blusys::app`
- capabilities are composed in `integration/`, not in `core/` or `ui/`
- capability events cross into product code only through the standard event bridge and reducer
- scaffold and product structure stay `main/`, `core/`, `ui/`, and `integration/`
- the three-tier architecture stays intact; capabilities live in the framework, built on top of `blusys_services`
- raw LVGL is allowed only inside custom widget implementations or explicit bounded custom view scope

## Current Repo Baseline

The repo already contains the Phase 0 through Phase 5 foundation needed for connected archetype work:

- `blusys::app` runtime, reducer model, profiles, and entry macros are real
- `BLUSYS_APP_MAIN_HEADLESS(spec)` provides a no-UI entry path
- `BLUSYS_APP_MAIN_DEVICE(spec, profile)` provides interactive entry with display profiles
- seven framework capabilities exist: connectivity, storage, provisioning, OTA, bluetooth, telemetry, diagnostics
- connectivity capability covers Wi-Fi, SNTP, mDNS, local control, and auto-reconnect
- telemetry capability provides buffered metric recording with user-defined delivery callback
- diagnostics capability provides periodic heap snapshots, chip metadata, and optional console
- OTA capability provides firmware update lifecycle with progress tracking and rollback protection
- provisioning capability provides BLE and SoftAP transport with QR payload support
- shared flows exist for connectivity, diagnostics, OTA, provisioning, boot, loading, error, status, and settings
- stock screens exist for about, status, and diagnostics
- `connected_headless` quickstart proves the basic headless reducer + connectivity pattern
- `connected_device` quickstart proves the basic interactive connected pattern
- interactive controller and panel references from Phase 5 prove the shell, navigation, and flow composition model

The main gap is not missing architecture. The main gap is missing productized connected reference apps that pressure-test the framework capabilities under realistic operational conditions: sustained connectivity, telemetry delivery, OTA lifecycle, diagnostics under load, and provisioning flows that feel like real product onboarding.

## Phase Deliverables

Phase 6 should produce the following outputs:

1. A primary `edge node` reference app.
2. A secondary `gateway/controller` starter or reference app.
3. Capability maturity improvements driven by real archetype pressure.
4. Connected-product guidance derived from the archetype work.
5. Validation coverage that proves headless and optional-interactive connected quality.

## Archetype Briefs

Before implementation work expands, both archetypes should be defined as concrete product starting points.

### Edge Node

This is the primary Phase 6 proof point.

- mode bias: headless-first
- input bias: no local UI by default; optional tiny status surface as a secondary variant
- presentation bias: operational, status-at-a-glance, minimal when present
- default theme direction: `operational_light` if an interactive variant is shown
- flow bias: provisioning, connectivity lifecycle, telemetry reporting, diagnostics, OTA maintenance
- capability bias: connectivity first, telemetry and OTA second, diagnostics and provisioning always present, storage for local buffering

The edge node represents the most common connected product shape: a device deployed in the field that collects data, maintains connectivity, accepts firmware updates, and reports its health. It must be operationally invisible when working correctly and operationally obvious when something is wrong.

The edge node reference should feel like a forkable field-deployed product, not a connectivity API demo.

Product loop:

1. Power on and provision (first run) or reconnect (subsequent runs).
2. Enter steady-state telemetry reporting.
3. Respond to connectivity interruptions with buffered retry and recovery.
4. Accept and apply OTA updates safely.
5. Report diagnostics on request through local control or telemetry metadata.
6. Surface operational state through log output and local control status endpoint.

### Gateway/Controller

This is the secondary Phase 6 proof point.

- mode bias: headless by default, with an optional interactive local operator surface
- input bias: optional local panel or operator display; encoder or button-based where present
- presentation bias: operational, dashboard-oriented, clear status hierarchy
- default theme direction: `operational_light`
- flow bias: orchestration status, connectivity and device management, diagnostics, settings, OTA maintenance, operator status
- capability bias: connectivity first, diagnostics and telemetry second, provisioning and OTA always present, local control for operator access, storage for configuration and state

The gateway represents a hub or coordinator device that manages connectivity, aggregates data from downstream devices or sensors, and provides an operational surface for local troubleshooting. It may run headless in rack-mounted deployments or with a local panel for field operators.

The gateway reference should prove that the same operating model supports both deployment modes without architecture changes.

Product loop:

1. Boot with stored configuration or enter provisioning.
2. Establish and maintain upstream connectivity.
3. Aggregate and forward telemetry from local sources.
4. Provide a local control and diagnostics surface.
5. Accept and apply OTA updates with rollback protection.
6. If interactive: display operational dashboard, connectivity status, and device health on a local panel.

## Workstreams

### Workstream A: Reference App Definition

Define a short implementation brief for each archetype before building deeper framework changes.

Each brief should lock:

- app purpose and primary operational loop
- core reducer states and action categories
- required capabilities and their configuration
- required flows (headless and interactive where applicable)
- connectivity failure and recovery behavior
- telemetry schema and delivery target
- OTA update trigger and lifecycle
- provisioning method and first-run behavior
- diagnostics surface (local control endpoint, console, optional UI)
- what makes the archetype feel operationally complete

This keeps the reference apps from drifting into capability API demos.

### Workstream B: Edge Node Reference

Build the primary edge node reference first.

Recommended scope:

- headless reducer with clear operational state machine: `idle`, `provisioning`, `connecting`, `connected`, `reporting`, `updating`, `error`
- connectivity capability with auto-reconnect, SNTP, mDNS, and local control
- telemetry capability with periodic metric collection and buffered delivery
- OTA capability with remote-triggered or periodic update checks
- diagnostics capability with health snapshots exposed through local control
- provisioning capability for first-run Wi-Fi setup
- storage capability for configuration persistence and telemetry buffering
- local control endpoint returning structured JSON status and accepting OTA trigger commands
- structured console logging with operational state transitions
- clear reducer-driven handling of connectivity loss, telemetry delivery failure, and OTA rollback

Implementation rules:

- keep operational state machine and business logic in `core/`
- keep capability setup, event bridges, and runtime-service integration in `integration/`
- `ui/` should exist but remain minimal for the headless-first variant; it may contain a tiny status screen for the optional interactive variant
- use all capabilities through the standard event bridge and reducer contract
- demonstrate sustained operation: the app should be designed to run indefinitely, not just complete a demo sequence
- demonstrate recovery: connectivity loss, telemetry backpressure, and OTA failure should all have clear reducer-driven recovery paths

This reference is the main pressure test for:

- headless operational quality
- capability composition under sustained load
- telemetry delivery maturity
- connectivity resilience and recovery
- OTA lifecycle in a realistic deployment context
- local control as the primary diagnostic and management surface
- provisioning as the real first-run experience

### Workstream C: Gateway/Controller Starter Or Reference

Build the gateway artifact after the edge node has exposed the first round of capability gaps.

Recommended scope:

- headless variant with the same operational state model as the edge node, plus orchestration state for downstream device awareness
- optional interactive variant using the shell, status screen, and diagnostics screen from the existing framework
- connectivity capability with upstream link management
- telemetry capability with aggregation from local sources
- OTA capability with the same lifecycle as the edge node
- diagnostics capability with richer system information for operator troubleshooting
- local control endpoint with expanded command surface for operator management
- storage capability for persistent configuration and operational logs
- if interactive: operational dashboard screen, connectivity status panel, and settings screen using stock framework components

This artifact should be intentionally less custom than the edge node reference. Its job is to prove that the same reducer, capability, and flow model scales to a coordinator role and supports optional local UI without architecture changes.

### Workstream D: Capability And Framework Refinement From Product Pressure

As the archetypes are built, feed only the smallest broadly useful improvements back into the framework.

Expected refinement areas:

- telemetry capability maturity: delivery retry, backoff, delivery confirmation lifecycle
- connectivity capability maturity: RSSI reporting, connection quality metrics, reconnect statistics
- diagnostics capability maturity: network diagnostics (gateway reachability, DNS resolution), task watermark reporting
- OTA capability maturity: version comparison, update-available detection, scheduled update windows
- local control maturity: structured command dispatch, action introspection, richer status responses
- provisioning flow maturity: factory reset flow, reprovisioning support
- headless operational patterns: structured log format, operational event classification, watchdog integration
- optional stock widgets for operational status: connection-quality indicator, telemetry-delivery indicator, uptime display

Rule: only generalize what clearly reduces product-owned burden across both archetypes or future connected forks. Do not build speculative abstractions for cloud platforms, device management protocols, or mesh topologies that are not exercised by the two reference apps.

### Workstream E: Docs, Examples, And Discoverability

When the references are credible, update the public surface so they become the main proof points for connected products.

Required changes:

- add archetype guidance explaining when to start from edge node vs gateway/controller
- add connected-product guidance explaining headless-first vs headless-with-optional-UI tradeoffs
- classify the new archetype apps correctly in `inventory.yml`
- keep one clear canonical connected quickstart path
- update capability docs when the archetypes reveal API drift or missing documentation
- update app and internals docs when the archetypes reveal naming or ownership guidance drift
- add operational runbook patterns showing how to diagnose, update, and recover a deployed connected product

## Proposed Repository Landing Pattern

The implementation should mirror the canonical product structure inside examples.

Recommended naming:

- `examples/quickstart/edge_node/` for the primary connected product entry
- `examples/reference/gateway/` for the secondary archetype

Selection rule:

- use `quickstart` for the edge node because it is the main recommended headless and connected product entry
- use `reference` for the gateway unless it also becomes part of the primary onboarding path

Each archetype app should reinforce:

- `main/` as the single local component
- `core/`, `ui/`, and `integration/` ownership boundaries
- `blusys::app` as the only recommended product-facing API

Internal structure for the edge node (headless-first):

```text
examples/quickstart/edge_node/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── core/
    │   ├── state.hpp
    │   ├── actions.hpp
    │   └── reducer.cpp
    ├── ui/
    │   └── (minimal or empty for headless-first)
    └── integration/
        ├── capabilities.hpp
        ├── event_bridge.cpp
        └── main.cpp
```

Internal structure for the gateway (headless + optional interactive):

```text
examples/reference/gateway/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    ├── core/
    │   ├── state.hpp
    │   ├── actions.hpp
    │   └── reducer.cpp
    ├── ui/
    │   ├── dashboard_screen.cpp  (optional interactive variant)
    │   └── screens.hpp
    └── integration/
        ├── capabilities.hpp
        ├── event_bridge.cpp
        └── main.cpp
```

## Milestone Order

The recommended execution order for Phase 6 is:

1. Write the edge node and gateway archetype briefs with locked scope.
2. Build the `edge node` reference first in headless-first form.
3. Validate edge node under sustained operation: connectivity cycling, telemetry delivery, OTA update, provisioning reset.
4. Record capability gaps discovered during edge node work.
5. Land the smallest reusable capability and framework refinements.
6. Build the `gateway/controller` starter or reference on the refined foundation.
7. Add the optional interactive variant for the gateway if the edge node work confirms the pattern is clean.
8. Update docs, inventory, and example classification.
9. Add validation and operational smoke coverage.

This order keeps capability generalization driven by real product pressure instead of speculative API expansion.

## Validation Plan

Phase 6 validation must go beyond compile success. Connected products must prove operational resilience, not just API correctness.

Required validation:

- `blusys host-build` for both connected archetypes (headless and interactive variants)
- targeted `blusys build` coverage on `esp32`, `esp32c3`, and `esp32s3`
- reducer tests for operational state machine transitions, connectivity recovery logic, and OTA lifecycle
- capability integration tests with simulated connectivity loss, telemetry backpressure, and OTA failure events
- sustained-operation smoke test: edge node should run for extended periods on host without state drift or resource leaks
- local control endpoint validation: status queries and command dispatch should return correct structured responses
- provisioning flow validation: first-run and reprovisioning paths should complete without manual intervention

Recommended manual checklist:

- operational state transitions are logged clearly and predictably
- connectivity loss triggers buffering and recovery, not crash or silent failure
- telemetry delivery resumes correctly after connectivity recovery
- OTA update lifecycle completes end-to-end with progress visibility
- local control endpoint accurately reflects current operational state
- provisioning flow leads to a working connected state without ambiguity
- if interactive gateway variant exists: dashboard reflects real operational state, not stale or placeholder data

## Exit Criteria

Phase 6 is complete when all of the following are true:

- the `edge node` reference feels dependable, connected, and operationally clear
- the `gateway/controller` reference feels reliable and operationally coherent
- both references use the same shared reducer, runtime, capability, and flow model
- neither reference requires repeated low-level runtime-service orchestration in app code
- the edge node runs headless-first and demonstrates sustained connected operation
- the gateway proves that the same operating model supports headless and optional interactive deployment
- diagnostics, telemetry, OTA, and status flows are reusable across both archetypes
- telemetry capability has been matured by real product needs, not speculative API surface
- a product team could fork either reference and quickly produce a differentiated connected product
- the docs and example surface clearly present these archetypes as real starting points rather than connectivity demos

## Risks To Watch

- overfitting capability APIs to one reference app instead of keeping them composable
- allowing the edge node reference to become a connectivity API walkthrough instead of an operational product
- allowing the gateway reference to collapse into a generic dashboard with a socket connection
- broadening the capability surface before a repeated product need drives it
- adding cloud-platform-specific integrations that should live outside the framework
- treating the optional interactive gateway variant as a mandatory deliverable rather than a proof-of-concept
- neglecting sustained-operation quality in favor of demo-path polish
- keeping `connected_headless` and `connected_device` quickstarts more prominent than the archetype references after the phase is complete

## Decision Rules During Phase 6

When implementation choices are ambiguous, prefer the option that:

- keeps archetypes as compositions rather than branches
- proves operational resilience rather than feature breadth
- removes product-owned orchestration burden rather than wrapping it
- keeps the headless path simple and the interactive path optional
- keeps capability contracts stable and composable
- preserves host-first iteration quality even for connected products
- improves both edge node and gateway reuse where possible
- keeps the change small and direct

## Relationship To Phase 5

Phase 6 is the connected counterpart to Phase 5. Where Phase 5 proved the platform can produce strong interactive products, Phase 6 proves the same platform can produce strong connected products.

The key architectural constraint is that Phase 6 must not introduce new runtime modes, separate connectivity frameworks, or alternative app models. The same reducer, capability, entry macro, and project structure model must power both interactive and connected products.

Where Phase 5 pressure-tested tokens, widgets, shell, and navigation, Phase 6 pressure-tests capabilities, operational flows, connectivity resilience, and headless operational quality. Both phases feed refinements back into the shared framework rather than building archetype-specific infrastructure.

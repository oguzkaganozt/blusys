# Blusys V7 Refactor PRD

## Status

Working draft. This document defines the target product shape for the v7 refactor and is the source of truth for scope, goals, and constraints.

## Problem Statement

Blusys already has a coherent internal three-tier architecture, but too much runtime wiring, UI setup, lifecycle management, and low-level orchestration still leaks into product code. The result is:

- application code is more verbose than it should be
- interactive products are not truly runnable by default
- product logic is too tightly coupled to framework internals and LVGL handles
- the public learning surface is too wide across examples, docs, and setup flows
- the repo is harder to maintain than necessary

The platform needs a breaking reset that makes product code dramatically simpler, pushes complexity into the framework, and preserves low-level power as an advanced path rather than the default path.

## Platform Framing

Blusys v7 is not being built as a generic ESP32 framework.

Blusys v7 is being built as the internal operating system for our product families.

That means the platform should optimize for the recurring products we actually design and ship, not for maximum generic breadth. The goal is to give every new product a shared operating model, shared lifecycle, shared interaction grammar, and shared service orchestration, while still allowing each product family to express its own identity.

## Vision

Blusys v7 should let a product team build a working MVP quickly with a minimal amount of C++ app code.

The product-facing path should be opinionated, small, and easy to scan:

- define app state
- define actions
- implement `update(ctx, state, action)`
- define screens and custom widgets if needed
- select optional hardware and service profiles
- run the same app on host, device, or headless targets

Everything else should be framework-owned unless there is a strong reason to expose it.

The target outcome is a platform that gives us:

- consumer products with tactile, expressive, high-character interaction and presentation
- industrial products with clear, reliable, operationally coherent flows
- one shared internal product operating model underneath both

Consumer inspiration is closer to Teenage Engineering: distinctive, tactile, concise, memorable.

Industrial inspiration is closer to Samsara: readable, dependable, operationally fluent, connected.

## Users

Primary users:

- product developers building MVPs and production apps on ESP32 targets
- internal maintainers evolving the framework and product path

Secondary users:

- advanced embedded users who need direct HAL, service, or LVGL access

## Product Goals

1. Make the default product path much smaller and simpler.
2. Make host-first interactive prototyping the default onboarding path.
3. Make headless-first hardware a first-class secondary path.
4. Build one shared operating model across our recurring product families.
5. Provide reusable interaction, connectivity, diagnostics, and configuration flows across products.
6. Keep the low-level layers available, but demote them from the recommended product workflow.
7. Make the repo easier to maintain by shrinking and curating the public surface.

## Non-Goals

- collapsing the three-tier architecture
- migrating HAL or services to C++ as part of this refactor
- preserving backwards compatibility with the current product-facing flow
- building a large reactive UI engine or virtual DOM
- forcing user widgets into a heavy inheritance hierarchy

## Locked Decisions

- The new product-facing namespace is `blusys::app`.
- The new product path is C++-only.
- App logic follows a reducer model: `update(ctx, state, action)`.
- Default onboarding is host-first interactive.
- Headless-first hardware is the secondary canonical path.
- The first canonical interactive hardware profile is a generic SPI ST7735 profile.
- The ST7735 profile must compile on ESP32, ESP32-C3, and ESP32-S3 from the first milestone; release readiness additionally requires real hardware validation on at least one target and staged validation across the others.
- Raw LVGL is allowed only inside custom widget implementations or explicit custom view scope.
- UI code must communicate outward only through `dispatch(...)` and app effects.
- Hardware and service profiles are code-first config, with Kconfig reserved for advanced tuning.
- Reducers mutate app state in place.
- HAL, services, `blusys_ui`, and low-level framework primitives remain as advanced escape hatches.

## Product Families

The platform should be optimized first for these recurring product families:

### 1. Interactive consumer devices

Examples:

- smart lights
- pomodoro timers
- smart speakers
- personal smart dashboards
- smart home control devices

Common qualities:

- strong visual and tactile identity
- display-first or display-plus-input interaction
- concise flows and feedback-rich interaction
- fast iteration on host before hardware tuning

### 2. Interactive control surfaces

Examples:

- home dashboards
- compact control panels
- operator-facing device screens

Common qualities:

- menu and screen-based navigation
- strong focus/input rules
- repeatable UI patterns across products

### 3. Headless connected appliances and nodes

Examples:

- smart home infrastructure devices
- sensor nodes
- appliance controllers

Common qualities:

- no display requirement in the main path
- strong local and remote lifecycle handling
- reliable connectivity and diagnostics

### 4. Industrial telemetry and control products

Examples:

- fleet management devices
- smart energy applications
- smart production line control systems
- industrial gateways and monitoring nodes

Common qualities:

- clear operational state
- dependable service orchestration
- diagnostics, control, update, and status visibility
- simpler integration into connected product systems

## Product Operating System Requirements

The platform should provide a shared internal operating model across those families.

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

## Design System Scope

The v7 reset is not only an API refactor. It is also the start of a reusable product design system.

The design system scope should include:

- theme tokens for color, spacing, radius, and type
- layout and page primitives
- focus and input behavior
- overlays, notifications, and transient feedback
- audio, haptic, and visual feedback semantics
- motion and transition defaults where appropriate
- reusable patterns for settings, lists, control surfaces, and status views

This does not mean every product looks the same. It means every product should share a consistent internal grammar and platform behavior, while preserving room for distinctive brand expression.

## Shared Operational Flows

The framework should progressively own common product flows instead of forcing each app to assemble them manually.

Priority flows include:

- boot and startup state
- loading and empty states
- error and recovery states
- settings and configuration
- provisioning and initial setup
- connectivity state and reconnect behavior
- local control and diagnostics
- OTA and maintenance hooks
- status, alerts, and confirmation feedback

## Target Product Model

The v7 product path should center on these app-owned concepts:

- `state`
- `action`
- `update(ctx, state, action)`
- screens and views
- optional hardware profiles
- optional service bundles

The framework should own these responsibilities:

- application boot and shutdown
- runtime loop and tick cadence
- navigation and route ownership
- feedback defaults and feedback plumbing
- LVGL lifecycle and lock discipline
- screen activation and overlay handling
- host, device, and headless adapters
- common input bridges
- common service orchestration for the default path
- reusable operating flows for common product behaviors

## Functional Requirements

### 1. App Runtime

The framework must provide a high-level app runtime that hides the current low-level framework plumbing from normal product code.

Required capabilities:

- define an app through a single `app_spec`
- run that app on host, device, or headless targets
- own the route stack and feedback sinks internally
- provide an `app_ctx` for dispatch, navigation, effects, and approved framework services
- keep the existing lower-level runtime primitives available as advanced/internal APIs

### 2. Product Family Alignment

The framework must be designed around recurring product families rather than around a flat inventory of modules.

Required capabilities:

- define a small set of canonical product archetypes and reference flows
- allow consumer and industrial products to share the same app/runtime model
- let products vary in presentation without varying in core operating model
- support both interactive and headless products as first-class peers

### 3. Interactive Product Path

The default interactive path must be runnable with no manual runtime plumbing and no manual LCD or UI lifecycle setup in normal product code.

Required capabilities:

- host-first quickstart that runs immediately
- framework-owned display and UI boot for the canonical path
- built-in navigation and overlay handling
- action-bound widgets and simple state bindings
- first-class ST7735 panel profile for SPI TFT onboarding

### 4. Headless Product Path

The headless path must use the same reducer and app model as the interactive path, with UI removed rather than a separate architecture.

Required capabilities:

- same app runtime core as the interactive path
- service and timer effects for headless flows
- local control and connectivity support through app-approved effects and bundles
- no UI or LVGL dependencies in the normal headless app path

### 5. View System

The framework must provide a simple, opinionated view layer that keeps common apps off raw LVGL while still permitting bounded customization.

Required capabilities:

- stock action-bound widgets for common controls
- simple bindings for text, value, enabled, and visible state
- page and layout helpers for common screens
- custom widget support through a formal contract rather than inheritance
- explicit custom LVGL blocks for advanced rendering inside a defined scope

### 6. Custom Widget Contract

Product-owned custom widgets must follow a common framework contract.

Required rules:

- public `config` or `props` struct as the widget interface
- raw LVGL remains inside the widget implementation
- visuals come from theme tokens only
- setters own widget state transitions when runtime mutation is needed
- widget behavior emits semantic callbacks or dispatches app actions
- interactive widgets support the standard focus and disabled model

### 7. Design System And Interaction Grammar

The framework must ship reusable design and interaction primitives that support both consumer and industrial products.

Required capabilities:

- default themes and layout primitives
- standard focus, input, and confirmation behavior
- reusable notification, overlay, and transient feedback patterns
- reusable settings, status, and control-surface patterns
- a coherent feedback model across visual, audio, and haptic channels

### 8. Service Bundles

The framework must provide higher-level bundles for common product needs so apps stop assembling low-level service lifecycles manually.

Ownership rule: these bundles belong in the framework-owned `blusys::app` path, composed on top of `blusys_services`. They must not shift orchestration responsibility back into product apps or sideways into the services tier.

Required initial bundles:

- connected-device bundle over Wi-Fi with optional local control, mDNS, and SNTP
- unified storage bundle over the current storage surfaces

Raw service APIs remain available but are not the recommended first path.

### 9. Shared Operational Flows

The framework must progressively ship reusable operating flows that reduce repeated app work across product families.

Ownership rule: these flows are framework-owned product behaviors built on top of services and HAL, not new service-tier responsibilities.

Required initial flow targets:

- startup, loading, and empty state handling
- settings and configuration entry points
- connectivity state and reconnect behavior
- diagnostics and local control hooks
- confirmation, alert, and maintenance patterns

### 10. Scaffold And CLI

The CLI and scaffold must reflect the new product model.

Required capabilities:

- `blusys create` generates a runnable host-first interactive app by default
- headless scaffold is equally first-class
- generated glue is clearly separated from user-owned app code
- scaffold templates live as real files, not inline heredocs inside a large script
- the first-run path uses the new app model only

### 11. Docs And Discoverability

Docs must be fast to scan from app surface to lower layers.

Required top-level flow:

- `Start`
- `App`
- `Services`
- `HAL + Drivers`
- `Internals`
- `Archive`

Docs should emphasize the recommended product path first and keep advanced surfaces clearly marked.

## User Experience Requirements

### Quickstart Experience

A user should be able to:

- create a new product
- run it on host immediately for the interactive path
- understand where app code lives and what to edit first
- find the headless quickstart without navigating through low-level examples first
- recognize which product family or archetype a new project should follow

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
2. Build for our recurring product families, not generic embedded breadth.
3. Prefer one strong default path over multiple equal-looking paths.
4. Keep low-level escape hatches available but visibly advanced.
5. Keep the implementation small and direct.
6. Avoid adding new abstractions unless they remove real product-code burden.

## Success Metrics

Product code metrics:

- scaffolded interactive app runs on host immediately
- scaffolded headless app is similarly minimal
- canonical product apps avoid low-level framework and UI lifecycle plumbing
- new products can map cleanly onto a known product family or reference flow

Repo and maintenance metrics:

- public examples are reduced to a curated set
- PR CI is materially smaller and faster
- docs are faster to scan and more consistent
- duplicate or stale public guidance is removed

Developer experience metrics:

- time from `blusys create` to first running host app is short and obvious
- users can find the right app-level guide before they need to read low-level module docs
- teams can reuse shared product flows instead of rebuilding them per product

## Risks

- over-designing the new app layer instead of keeping it minimal
- building a cleaner framework that is still too generic for our real product work
- allowing raw LVGL or raw services to leak back into the default path
- carrying old and new product paths in parallel for too long
- delaying the scaffold rewrite until after too many lower-level changes pile up

## Out Of Scope For The First Cut

- broad service-family redesigns beyond the initial bundles
- large-scale HAL API redesign
- advanced board-profile catalog beyond the first canonical interactive profile
- broad changes to target support policy

## Release Criteria

The v7 reset is ready for cutover when:

- `blusys::app` is the only recommended product-facing API
- interactive and headless quickstarts both use the new app model
- the generic SPI ST7735 profile exists and builds on all three targets
- the scaffold and getting-started flow use the new model
- the old public app path is archived or clearly demoted
- the platform clearly supports our core product families through shared app and operating flows

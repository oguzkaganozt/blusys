# PRD release criteria — status

This page tracks the **`PRD.md`** (repository root) **Release Criteria** (product-platform reset cutover) against the current tree. It is a living checklist for maintainers, not a substitute for CI.

| Criterion (PRD) | Status | Notes |
|-----------------|--------|--------|
| `blusys::app` is the only **recommended** product-facing API | **Met** — public docs center on [App](../app/index.md); advanced HAL/services remain documented as lower layers. |
| Interactive and headless quickstarts both use the **new app model** | **Met** — [Interactive Quickstart](../start/quickstart-interactive.md), [Headless Quickstart](../start/quickstart-headless.md), archetype starters. |
| Generic SPI **ST7735** profile exists and builds on **esp32, esp32c3, esp32s3** | **Met** — profile in framework; CI builds curated examples + Phase 7 variants on all three targets. |
| Scaffold and getting-started flow use the **new model** | **Met** — `blusys create --archetype …`; [Archetype Starters](../start/archetypes.md). |
| Default scaffold uses **`main/`** as single local component and **`core/` / `ui/` / `integration/`** | **Met** — enforced by scaffold; gateway headless omits `ui/` only where the archetype is headless-first. |
| Product-facing language uses **capabilities** (not **bundles**) | **Met** — terminology cutover tracked in Phase 0 exit; remaining “bundle” mentions are historical internals only. |
| Platform supports **four canonical archetypes** through shared app and operating flows | **Met** — examples + `blusys create`; [Phase 8 exit](phase-8-exit.md). |

## PRD §9 validation (ongoing)

Ship-time quality also depends on [Validation and host loop](../app/validation-host-loop.md) (reducer smokes, capability checks, scaffold smoke, CI). That section is **continuously improved**; it is not a single “gate” in this table.

## When to update this page

Update when release criteria change in `PRD.md` or when a criterion regresses (e.g. a new public API bypasses `blusys::app`).

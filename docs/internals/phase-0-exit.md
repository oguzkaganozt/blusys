# Phase 0 Exit Status

This document records closure against `phase-0-v7-foundation-plan.md` and the Phase 0 exit criteria in `ROADMAP.md`.

## Exit criteria (ROADMAP)

| Criterion | Status |
|-----------|--------|
| One unambiguous recommended product path | **Met** — `blusys::app`, reducer model, `docs/start/` + `docs/app/` |
| Quickstarts, scaffold, docs, and code point at the same API | **Met** — ongoing maintenance; scaffold matches `core/` / `ui/` / `integration/` |
| Every scaffolded project uses the same fixed directory layout | **Met** — enforced by `blusys create` |
| Every scaffolded project uses `main/` as the single local component | **Met** |
| Interactive routing is framework-owned | **Met** — `screen_router` / `screen_registry` bind as `route_sink`; navigation via `app_ctx` |
| Input behavior is stable as a default path | **Met** — host SDL and device bridges post the same `intent` stream; focus reattach on screen change |
| Public surface uses `capabilities` (not `bundles`) for readiness/status | **Met** — `capability_ready` product API |
| Standard capability contract documented | **Met** — `docs/app/capabilities.md` + `capability-composition.md` |

## Deliverables (foundation plan)

| # | Deliverable | Status |
|---|-------------|--------|
| 1 | Framework-owned route stack and screen ownership | **Met** (see `screen_router`, `route` commands) |
| 2 | Unified input and focus (host + hardware) | **Met** (intents + `input_bridge` / host keyboard path) |
| 3 | Host and headless profile surfaces | **Met** — `host_profile`, `device_profile`, `headless_profile` + `BLUSYS_APP_MAIN_HEADLESS_PROFILE` |
| 4 | Scaffold aligned to current app API | **Met** |
| 5–6 | `core/` / `ui/` / `integration/`, single `main/` component | **Met** |
| 7 | Quickstarts / docs use capabilities terminology | **Met** |
| 8 | Documented capability contract | **Met** |
| 9 | Low-level framework examples not in `examples/quickstart/` | **Met** — `framework_device_basic`, `connected_device`, `connected_headless` live under `examples/reference/` |
| 10 | Scaffold dependency pinning | **Met** — `idf_component.yml` from `blusys create`, `BLUSYS_SCAFFOLD_PLATFORM_VERSION` |

## Intentionally deferred (broader roadmap)

These were called out as gaps in the plan’s baseline but are **not** Phase 0 closure requirements: larger theme system, expanded stock widgets, shared operational flows library, and product-behavior-centric CI beyond builds.

## Validation

Run proportionally after changes: `blusys host-build` (interactive quickstart), `blusys build` for ST7735 targets, headless quickstart build, `python scripts/check-inventory.py`, `mkdocs build --strict`, `blusys lint` (layering).

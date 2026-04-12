# Maintaining Blusys

Quick reference for contributors: where the platform lives, and what to run before merging or tagging a release.

## Three ESP-IDF components

| Directory | Component name (`REQUIRES`) | Role |
|-----------|----------------------------|------|
| `components/blusys_hal/` | `blusys_hal` | HAL + drivers (C); public headers under `include/blusys/` |
| `components/blusys_services/` | `blusys_services` | Runtime services (C) |
| `components/blusys_framework/` | `blusys_framework` | Product framework (C++) |

Dependency direction: `blusys_framework` → `blusys_services` → `blusys_hal` → ESP-IDF. The **`blusys/`** include namespace is shared across tiers; only the **on-disk component folder** for HAL is `blusys_hal`.

## Suggested reading order

1. Repository root `README.md` — product foundations (same scope as the docs [Home](../index.md) landing page)  
2. [Architecture](architecture.md) — tiers and layering  
3. [Guidelines](guidelines.md) — API and workflow  
4. Repository root `inventory.yml` — modules, examples, docs (CI manifest)

**Framework UI audits:** [`refinement-plan.md`](https://github.com/oguzkaganozt/blusys/blob/main/refinement-plan.md) at the repository root — widget slot-pool consolidation, `set_widget_disabled`, and related internal rationale (design history; implementation status is summarized at the top of that file).

## Pre-merge / pre-release checks

Align with `.github/workflows/ci.yml` and repository root `CLAUDE.md`:

- `./blusys lint` — layering (`scripts/lint-layering.sh`) + framework UI source list consistency  
- `python3 scripts/check-inventory.py`  
- `python3 scripts/check-framework-ui-sources.py` (after CMake or widget path changes)  
- `mkdocs build --strict` (any doc or `mkdocs.yml` change)  
- Representative builds: `blusys host-build` (e.g. `scripts/host` or a quickstart example with `host/`), and `blusys build` on at least one HAL validation example when components change  

For broader changes, also run `python3 scripts/check-product-layout.py` and `scripts/scaffold-smoke.sh` as appropriate.

# Blusys Manifest-First Scaffold Plan

## Objective

Make blusys easier to start, learn, and extend by making `blusys.project.yml` the single source of truth for product shape and by shrinking the default scaffold to the minimum.

## Target State

- `blusys.project.yml` defines `schema` (initially `1`), `interface`, `capabilities`, `flows`, `profiles`, and `policies`.
- `interactive` starter scaffolds: `main/app_main.cpp` plus an empty `main/ui/` directory.
- `headless` starter scaffold: `main/app_main.cpp` only.
- No mandatory `core/` or `platform/` starter folders.
- No runtime YAML parsing on device.
- The generated app entrypoint stays thin and derives its wiring from the manifest.
- `schema` is versioned from day one; unknown schema versions fail fast.
- Profiles are built-in factory references with small overrides in v1, not standalone versioned manifest objects.

## Guiding Principles

1. Manifest first, code second.
2. Thin defaults, no ceremony.
3. Generated wiring, hand-written behavior.
4. Keep runtime overhead at zero.
5. Keep advanced escape hatches available, but not default.
6. Validate early and fail loud.
7. Preserve the existing framework layering rules.

## Phase 1 - Lock the Manifest Contract

Goal: define the v1 product-shape schema and stop ambiguity early.

Scope:

- Freeze the manifest field set: `schema`, `interface`, `capabilities`, `flows`, `profiles`, `policies`.
- Treat `schema: 1` as the initial contract.
- Define the allowed values for `interface`: `headless`, `interactive`.
- Define interface semantics explicitly: `headless` means no direct local control surface; `interactive` means direct local UI exists.
- Keep the interface axis about local UI presence only; display technology, input hardware, physical size, and product form factor stay in profiles and theme/layout hints.
- Allow capability entries to carry small typed config blocks for stock capabilities.
- Profiles reference built-in framework factories plus small override blocks for display tech, input hardware, orientation, density, and pin maps.
- An explicit profile selection can override the interface's default family; the interface never overrides profile fields.
- Profiles do not carry capabilities or app logic.
- Keep flows as explicit identifiers for stock or custom flow names.
- Keep policies as a small list of cross-cutting flags.

Deliverables:

- A schema reference page for `blusys.project.yml`.
- A schema versioning and migration note.
- A profile schema reference page covering built-in factory references, override rules, and precedence.
- Updated `blusys create --list` output.
- Validation rules for legal combinations.
- Example manifests for headless and interactive starters.
- A compatibility policy for deprecated `none` / `controls` / `touch` labels if temporary aliases are needed during migration.

Exit criteria:

- One canonical manifest shape exists.
- Invalid combinations fail with clear errors.
- Product shape is no longer duplicated across docs and templates.

## Phase 2 - Reduce the Default Scaffold

Goal: lower friction for simple apps.

Scope:

- `interactive` starters generate `main/app_main.cpp` and an empty `main/ui/` directory.
- `headless` starters generate only `main/app_main.cpp`.
- Stop generating mandatory `core/` and `platform/` folders in the starter.
- Keep split-out helper files optional, not starter requirements.

Deliverables:

- Updated scaffold templates.
- Updated starter docs and manifest fixtures.
- A small app entrypoint template that stays readable.
- No new public quickstart example trees; the manifest-first starter docs are the canonical onboarding path.

Exit criteria:

- A new project can be created without being forced into a large folder tree.
- First edits stay in one file for small apps.

## Phase 3 - Derive Wiring From the Manifest

Goal: remove duplicate product-shape declarations.

Scope:

- Generate the app wiring from `blusys.project.yml`.
- Map `schema` and `interface` to the correct default profile family and scaffold mode, then apply any explicit profile override.
- Map `capabilities` to capability registration.
- Map `flows` to stock flow registration.
- Map `policies` to build/config overlays.
- Validate that checked-in wiring matches the manifest.

Deliverables:

- Generator logic that emits or updates the thin app entrypoint.
- Manifest drift checks.
- A clear failure message when the manifest and app wiring disagree.
- A deprecated-name warning path for `none` / `controls` / `touch` only if compatibility aliases are required during migration.

Exit criteria:

- Product shape is declared once.
- No manual duplication of interface, capability, flow, profile, or policy lists.

## Phase 4 - Add Small Presets, Not Bigger Defaults

Goal: keep the starter compact while still giving good defaults.

Scope:

- Provide curated presets for `headless` and `interactive` starter shapes.
- Keep the preset set small and opinionated.
- Include only common capability packs, not every possible combination.
- Keep custom flows and custom capabilities as explicit opt-in paths.

Deliverables:

- Presets exposed through `blusys create`.
- Manifest examples for headless and interactive starters.
- Starter mappings for the supported interface presets.

Exit criteria:

- A developer can create a credible starter with one command.
- The platform does not gain a large template matrix.

## Phase 5 - Rewrite Docs Around the Manifest

Goal: reduce the learning curve by removing concept duplication.

Scope:

- Make the manifest the first thing a new developer sees.
- Explain reducer logic after the manifest, not before it.
- Keep `app_main.cpp` as the default mental model; split-out helper files are optional and only introduced when the entrypoint stops being readable.
- Add a short "where does this belong?" guide for behavior, UI, and wiring.

Deliverables:

- Updated `docs/start/*` pages.
- Updated `docs/app/*` pages.
- Simplified starter docs and fixtures.
- A small glossary for platform terms.

Exit criteria:

- A new developer can understand product shape without reading architecture docs first.
- The docs path is shorter and more task-first.

## Phase 6 - Tighten Validation and CI

Goal: make the new model durable.

Scope:

- Add manifest validation to `blusys validate`.
- Extend scaffold smoke tests to cover the new minimal defaults.
- Keep product-layout checks aligned with the new scaffold.
- Verify the starter matrix for the supported targets.

Deliverables:

- Validation for manifest schema and allowed combinations.
- A tiny committed starter fixture under `tests/fixtures/manifest-starter/`.
- Scaffold smoke coverage that generates temp projects from the manifest-first starter and runs them on host.
- CI updates for the new defaults.

Exit criteria:

- Invalid manifests fail before merge.
- Starter regressions are caught automatically.

## Phase 7 - Preserve Flexibility Without Reintroducing Bloat

Goal: support larger products without forcing extra structure on simple ones.

Scope:

- Document optional growth paths for extracting UI or helper code from `app_main.cpp` only when needed.
- Keep advanced capability and flow authoring documented, but off the default path.
- Keep escape hatches explicit and small.

Deliverables:

- A growth-path guide for small, medium, and larger products.
- Examples showing when to split logic out of `app_main.cpp`.

Exit criteria:

- Simple apps stay simple.
- Complex apps still have a clear path forward.

## Migration Strategy

1. Land the versioned manifest schema and validation first.
2. Add the new `headless` / `interactive` interface names and semantics.
3. Update scaffold generation, docs, inventory, and validators together.
4. Use warning-only compatibility aliases only if needed, and keep them time-boxed to one migration window.
5. Accept deprecated interface names only in CLI creation, inventory ingestion, and validation during the migration window; emit warnings.
6. Remove deprecated interface names and any shims after the migration window.
7. Keep the manifest-first starter docs as the canonical onboarding path; do not reintroduce public quickstart trees.
8. Tighten validation and CI after the new path is stable.

## Guardrails

- Do not add runtime manifest parsing on device.
- Do not introduce a new mandatory folder layer.
- Do not create a large abstraction framework around the manifest.
- Do not duplicate product shape across multiple config files.
- Do not weaken the current layering rules.
- Do not make the default path more complex than the app itself.

## Success Metrics

- Default starter is one manifest plus one source file, with `main/ui/` only for interactive apps.
- First host build requires fewer edits and fewer concepts.
- Adding a capability or flow is a manifest edit, not a multi-file refactor.
- New developer onboarding requires less documentation and less architecture knowledge upfront.
- The platform stays small because the CLI and templates do the work, not the runtime.

## Recommended Implementation Order By ROI

1. Manifest schema and validation.
2. Minimal scaffold templates.
3. Manifest-derived wiring.
4. Starter presets.
5. Documentation rewrite.
6. CI and smoke-test expansion.
7. Optional growth-path docs.

# Blusys Manifest-First Scaffold Plan

## Objective

Make blusys easier to start, learn, and extend by making `blusys.project.yml` the single compile-time source of truth for product shape, and by shrinking the default scaffold to the minimum a working app needs.

## Status

Supersedes the v1 draft. The `manifest-first-scaffold` branch has landed partial progress on phases 1 and 5; phases 2 and 3 are the critical path. "Current Status and Gaps" below tracks what is done versus remaining.

## Target State

A default `headless` starter is:

- `blusys.project.yml` — 5–10 lines, product shape only.
- `main/app_main.cpp` — hand-written, thin entrypoint with a visible custom-init zone.
- No `main/core/`, no `main/platform/`, no generated C++ checked in.

A default `interactive` starter adds:

- `main/ui/app_ui.cpp` — small sample component tree.
- `main/ui/CMakeLists.txt`.

All product shape (interface, capabilities, profile, policies) lives in the manifest. Wiring derivable from the manifest is generated into `build/` and never checked in. The runtime never parses YAML.

## Design Decisions

Ten decisions, locked.

**D1. Field name is `schema`, not `version`.** The field names the contract version of the manifest, not the app or product version. One mechanical rename across templates, catalog, docs, and fixtures.

**D2. Generated wiring is never checked in.** At build time, a CMake pre-build step translates `blusys.project.yml` into `build/generated/blusys_app_spec.h` (and any companion source). The hand-written `app_main.cpp` includes that header and calls `blusys::run(blusys::generated::kAppSpec)` inside an explicit `app_main()` function that also hosts product-specific init. One source of truth, no drift check.

**D3. Profiles are named references in schema v1. Overrides come later.** Manifest allows `profile: <factory_name>` as a single optional string. Typed override blocks (orientation, pin maps, density) are deferred to schema v2. Factories stay in C++.

**D4. Schema is versioned; unknown versions hard-fail.** `blusys validate` and `blusys create` reject manifests with an unrecognized `schema:` value, printing the supported range and a migration hint. Additive changes (new optional fields) do not bump the major. Removing or repurposing a field does bump.

**D5. Manifest fields are a closed set.** `blusys validate` rejects unknown top-level keys. Typos fail loud, early.

**D6. No compatibility shims for `none` / `controls` / `touch`.** Those interface values never existed in this repo. The v1 plan's migration section around them was fiction and is removed.

**D7. Manifest = product shape, not runtime config.** Runtime values (WiFi SSID, MQTT endpoints, OTA URLs) live in NVS (runtime-writable) and Kconfig / `sdkconfig.defaults` (compile-time defaults). No secrets in git. No nested config blocks in the manifest.

**D8. Interface collapses to `interactive | headless`.** The three-value `handheld | surface | headless` set in today's catalog has two mechanically identical entries (handheld and surface are both `build_ui: true` with identical wiring). Collapsing matches the real functional axis — local UI yes or no. Form-factor personality moves to profile selection, not a parallel interface axis.

**D9. No mandated growth triggers.** The framework imposes no local folder structure beyond `main/`. `core/` and `platform/` are not reintroduced as conventions. Developers organize their code however they want. The plan does not prescribe when or how to split `app_main.cpp`.

**D10. Escape hatch: override-in-code.** The generated `AppSpec` is a value, not a jail. Developers can use it as-is, copy it and modify, or bypass it and hand-write their own `AppSpec` in `app_main.cpp`. The manifest validator still runs; CI still gates on schema. The generated spec is the 90% path, not the only path.

## Manifest v1 Shape

```yaml
schema: 1
interface: interactive | headless
capabilities: [string, ...]
profile: string | null
policies: [string, ...]
```

- `interface` selects whether a local UI is built.
- `capabilities` names stock capability packs from the catalog (`connectivity`, `bluetooth`, `usb`, `telemetry`, `ota`, `lan_control`, `storage`, `diagnostics` today).
- `profile` names a built-in platform profile factory (e.g. `st7735_160x128`). `null` means use the interface's default profile.
- `policies` names cross-cutting flags (`low_power` today).

`flows:` is deferred. When stock flows become real registered entities, the field will be added as an additive change (no schema bump per D4).

No nested blocks in v1. Adding nested config is a schema v2 change.

## Starter Footprint

**Headless — 9 files, 1 hand-written source:**

```
starter/
├── blusys.project.yml
├── CMakeLists.txt
├── README.md                   (short stub, 5–10 lines)
├── sdkconfig.defaults
├── sdkconfig.qemu
├── main/
│   ├── CMakeLists.txt
│   ├── idf_component.yml
│   └── app_main.cpp            (β shape, ~8 lines)
└── host/
    └── CMakeLists.txt
```

**Interactive — 11 files, 2 hand-written sources:**

Adds:

```
main/ui/
├── CMakeLists.txt
└── app_ui.cpp                  (small sample component tree)
```

## `app_main.cpp` Shape

```cpp
#include "blusys_app_spec.h"

extern "C" void app_main() {
    // --- product-specific init (NVS, logging, hardware) ---

    blusys::run(blusys::generated::kAppSpec);
}
```

Always explicit. Always a visible init zone. No macro-to-function upgrade step when the developer adds startup code.

## Phases

Seven phases, ROI-ordered. Each has concrete exit criteria.

### Phase 1 — Schema contract, validator, interface rename

Goal: make the manifest contract decisive and land all rename work in one atomic migration.

Scope:

- Rename `version` → `schema` in templates, catalog, docs, fixtures, smoke matrix.
- Collapse catalog `interfaces` from three entries to two (`interactive`, `headless`); migrate handheld/surface fixtures to `interactive`.
- Add `profile` field to the schema (optional string, null default).
- `blusys validate` implements the 5-rule check set (see Validation Rules below).
- `blusys create` runs the validator on generated manifests before writing.

Exit criteria:

- `blusys validate` rejects a manifest with a typo in any field name.
- `blusys validate` rejects `schema: 99` with a supported-range message.
- `blusys validate` rejects `interface: handheld` with a clear message naming valid values.
- Every checked-in fixture passes validation.
- `version:` appears nowhere in the repo outside historical git log.

### Phase 2 — Shrink the scaffold

Goal: match the target state for default starters.

Scope:

- `blusys create --interface headless` produces exactly the 9-file footprint above.
- `blusys create --interface interactive` adds `main/ui/app_ui.cpp` and its `CMakeLists.txt`.
- No `main/core/` or `main/platform/` folders generated by default.

Exit criteria:

- `blusys create --interface headless tmp/x && find tmp/x/main -type f | wc -l` = 3 (`app_main.cpp` + `CMakeLists.txt` + `idf_component.yml`).
- Generated project host-builds green with no hand edits.
- Scaffold smoke matrix passes on the new layout.

### Phase 3 — Generate wiring from the manifest

Goal: remove the duplicate product-shape declaration that lives in hand-written `app_main.cpp` templates today.

Scope:

- Add `blusys gen-spec` subcommand that translates `blusys.project.yml` into `build/generated/blusys_app_spec.h` (and any companion source file).
- Hook `gen-spec` into CMake via `add_custom_command` so it runs before `app_main.cpp` compiles.
- Generated file carries a single `constexpr blusys::AppSpec kAppSpec` (or equivalent) containing: capability instances, profile reference, policy flags.
- Generated file is gitignored; CI fails if it is ever committed.
- Hand-written `app_main.cpp` reduces to the β shape above.

Exit criteria:

- Editing `capabilities:` in the manifest and rebuilding picks up the new capability with zero hand edits elsewhere.
- Two consecutive builds from the same manifest produce identical generated output.
- `app_main.cpp` diff between headless and interactive starters is ≤5 lines.

### Phase 4 — Small preset surface

Goal: keep starter choices narrow.

Scope:

- `blusys create --interface {interactive|headless}` with optional `--with cap1,cap2`.
- Curated capability packs stay small and opinionated — no exhaustive matrix.
- `blusys create --list` output fits on one terminal screen.

Exit criteria:

- `blusys create --list` output is ≤25 lines.
- Every listed preset has smoke coverage.

### Phase 5 — Docs and cold-onboarding proof

Goal: verify the docs actually work for a first-time developer.

Scope:

- `docs/start/*` leads with the manifest, not architecture.
- All references to `handheld` / `surface` replaced with `interactive` (or removed).
- Docs describe the escape-hatch path (override-in-code) under an Advanced section.
- Cold-onboarding test: scripted walkthrough, or a developer unfamiliar with blusys, produces a host-running headless app in ≤15 minutes from `git clone`.

Exit criteria:

- Cold-onboarding test passes once per release with a developer new to blusys.
- Every doc example is covered by a smoke test.

### Phase 6 — CI validation and drift prevention

Goal: make the new model durable.

Scope:

- `blusys validate` runs in pre-commit and CI on every manifest in the repo.
- Scaffold smoke regenerates and host-builds every starter on every PR.
- CI fails if any path under `build/generated/**` is ever committed.

Exit criteria:

- A PR introducing an invalid manifest fails CI in under 60 seconds.
- A PR committing a generated spec file fails with a clear message.

### Phase 7 — Escape hatches and reference

Goal: make the override path visible without advertising it on the default path.

Scope:

- Document the override-in-code pattern with a complete example.
- One reference example under `examples/reference/` exercises the override (e.g. a product with a one-off custom capability).
- Document custom-capability authoring and link from the override doc.

Exit criteria:

- Override reference example builds and runs on host.
- Default-path docs do not mention override-in-code; it lives under Advanced only.

## Validation Rules

`blusys validate` checks five things, all data-driven from the catalog:

1. **Schema version known** — `schema: 99` → error with supported range.
2. **Top-level keys in the allow-list** — typos (`capabilties:`) → error naming the unknown key.
3. **Values well-typed** — lists are lists, strings are strings; wrong shape → type error.
4. **Values in catalog** — unknown `interface`, `capability`, `policy`, or `profile` → error listing valid options.
5. **Required fields present** — `schema` and `interface` are mandatory; others optional.

Intentionally excluded:

- Hand-coded illegal-combination rules. If a combination is truly invalid (e.g., a display-requiring profile on a headless interface), the catalog encodes it — profiles declare which interfaces they support, and the validator reads the catalog.
- Drift checks against checked-in generated files — moot per D2.
- Semantic or runtime checks beyond structure.

Invocation sites:

- `blusys create` — validates the manifest it just wrote before returning success.
- `blusys validate` — on-demand, from the CLI.
- Pre-commit hook — any committed manifest.
- CI — every PR touching a manifest.

## Escape Hatches

Documented, supported, off the default path.

**Override-in-code.** The developer defines their own `AppSpec` in `app_main.cpp` (either copying the generated one and modifying, or hand-writing from scratch) and passes it to `blusys::run`. The manifest validator still runs — shape declarations remain authoritative — but the generated spec is bypassed for this build.

**Custom capabilities and profiles.** Authored in C++ under `components/`. To appear in the manifest, they must be added to the catalog; otherwise they are invoked via override-in-code only.

**Local code organization.** Developers may split `app_main.cpp` into additional files under `main/` whenever they choose. The framework imposes no convention on when, how, or into what folder names (D9).

## Schema Evolution

- `schema: 1` is the initial contract.
- Additive changes (new optional fields) keep `schema: 1`.
- Removing or repurposing a field bumps to `schema: 2`. The CLI refuses to generate `schema: 2` manifests until a migration path ships.
- `blusys validate` and `blusys create` reject unknown schema numbers with a supported-range message.
- No runtime compatibility shims. Migration happens at manifest-authoring time.

## Guardrails

- Do not add runtime YAML parsing on device.
- Do not check in generated wiring.
- Do not introduce a mandatory folder layer beyond `main/`.
- Do not build a large abstraction framework around the manifest — keep the schema flat.
- Do not duplicate product shape across multiple config files.
- Do not weaken existing framework layering rules.
- Do not make the default path more complex than the app itself.
- Do not prescribe local code organization (when to split files, subfolder naming, etc.).

## Success Metrics

Concrete, pass/fail:

- **Headless starter size:** 1 hand-written source file, no `core/` or `platform/` folders.
- **Interactive starter size:** 2 hand-written source files.
- **Cold-onboarding time:** new developer produces a host-running app in ≤15 minutes from `git clone`.
- **Capability edit:** adding a capability is a one-line manifest change with zero edits elsewhere.
- **Manifest typo survival:** zero typos reach build; all fail validation.
- **Repo size growth:** no new mandatory folders introduced on the default path.
- **Generated artifact drift:** zero — enforced by CI.

## Current Status and Gaps

Done on the `manifest-first-scaffold` branch (commit 37f9f73):

- Manifest file exists with `version`, `interface`, `capabilities`, `policies`.
- Docs in `docs/start/` and `docs/app/` lead with the manifest.
- Quickstart example trees removed in favor of `blusys create`.
- Scaffold smoke matrix exists and passes.

Not done (blocks this plan):

- `version` not yet renamed to `schema` (D1).
- `profile` not yet in the manifest schema (D3).
- Interface still carries three values in the catalog (D8).
- Scaffold still emits `main/core/` and `main/platform/` folders — contradicts Phase 2.
- Wiring is generated from CLI args at scaffold time, not from the manifest at build time — contradicts Phase 3 and D2.
- `blusys validate` does not yet enforce the closed field set or schema version (D4, D5).


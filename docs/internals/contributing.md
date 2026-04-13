# Contributing

Concrete checks, CI jobs, and local validation for Blusys contributors. Read [Architecture](architecture.md) and [Guidelines](guidelines.md) first for the rules these checks enforce.

## Local checks

Run proportionate checks for the scope of your change:

| Change touches… | Run |
|-----------------|-----|
| Anything under `components/` | `./blusys lint` (layering + framework UI sources + host-bridge spine) |
| `cmake/blusys_framework_ui_sources.cmake`, widget paths | `python3 scripts/check-framework-ui-sources.py` |
| `inventory.yml`, new modules/examples/docs | `python3 scripts/check-inventory.py` |
| Product-shaped `main/` layouts | `python3 scripts/check-product-layout.py` |
| Example paths | `python3 scripts/example-health.py` |
| `blusys create`, templates, archetype host CMake | `scripts/scaffold-smoke.sh` |
| Docs, `mkdocs.yml` | `mkdocs build --strict` |
| Components or examples | `blusys build` / `blusys host-build` |

### Host tests

From `scripts/host/build-host/` (configure with `blusys host-build`):

```sh
ctest                               # all host tests (spine, snapshot buffer)
```

Sanitizer variant:

```sh
cmake -S scripts/host -B scripts/host/build-host-asan -DBLUSYS_HOST_SANITIZE=ON
cmake --build scripts/host/build-host-asan
ctest --test-dir scripts/host/build-host-asan
```

Mirrors CI’s “Host spine tests (sanitizer)” job.

### Device and QEMU

```sh
blusys build <project> <esp32|esp32c3|esp32s3>
blusys qemu --serial-only <project> <qemu32|qemu32c3|qemu32s3>
```

After app-flow or integration changes, prioritize: host interactive, headless, scaffold, and ST7735-class device builds on all three silicon targets.

## CI jobs (PR)

Workflow: `.github/workflows/ci.yml`. Triggered by push/PR to `main` on watched paths.

| Stage | What it runs |
|-------|--------------|
| Lint | `scripts/lint-layering.sh` + `scripts/check-framework-ui-sources.py` |
| Inventory | `scripts/check-inventory.py` + `scripts/check-product-layout.py` |
| Host smoke | Configure/build `scripts/host`, run smokes, `ctest`, sanitizer build/run; build `edge_node` + `gateway` host examples; `scaffold-smoke.sh` |
| Docs | `mkdocs build --strict` |
| Device builds | `scripts/build-from-inventory.sh` with `ci_pr=true` across the target matrix |
| Display variants | `scripts/build-display-variants.sh` |
| QEMU smokes | Subset via `scripts/qemu-test.sh` |

PR coverage runs only examples flagged `ci_pr: true` in `inventory.yml`. Nightly (`.github/workflows/nightly.yml`) runs broader example builds.

## Adding a module, example, or doc

1. Add the code/content.
2. Register it in `inventory.yml` (see existing entries for the schema; `layout_exempt` / `product_layout` flags documented at the top of the file).
3. If it’s a doc, add the nav entry in `mkdocs.yml`.
4. Run `python3 scripts/check-inventory.py` and `mkdocs build --strict`.
5. If it’s a product-shaped example, run `python3 scripts/check-product-layout.py`.
6. If it’s a new archetype/template, run `scripts/scaffold-smoke.sh`.

## Tier discipline

- HAL and services are C and must not expose ESP-IDF types in public headers.
- Framework is C++ (`-fno-exceptions -fno-rtti`), no allocation after init, no RAII over LVGL or ESP-IDF handles.
- Reverse dependencies across tiers are forbidden; `blusys lint` enforces the HAL/drivers boundary.

## Release

Single tag covers all three components. Before tagging:

- `./blusys lint` clean
- `python3 scripts/check-inventory.py` clean
- `mkdocs build --strict` clean
- Representative `blusys build` across `esp32`, `esp32c3`, `esp32s3`
- `BLUSYS_VERSION_STRING` in `components/blusys_hal/include/blusys/version.h` matches the tag
- `blusys version` prints the new string

## See also

- [Architecture](architecture.md)
- [Guidelines](guidelines.md) — `Maintaining` and `Integration baseline` sections
- Repository root `CLAUDE.md` — agent-facing conventions

# AGENTS.md

## What This Repo Is

- `Blusys HAL` is a C ESP-IDF component in `components/blusys/`.
- Supported targets are exactly: `esp32`, `esp32c3`, `esp32s3`.
- `v1.0.0` is released; next planned work starts at `V2` in `docs/roadmap.md` and `PROGRESS.md`.

## Source Of Truth

Read these first before changing API or project status:

- `README.md`
- `PROGRESS.md`
- `docs/architecture.md`
- `docs/api-design-rules.md`
- `docs/roadmap.md`

Trust scripts and CMake over prose if they disagree.

## Build And Flash Shortcuts

All commands go through the unified `./blusys.sh` script:

- build one example: `./blusys.sh build examples/<name> [esp32|esp32c3|esp32s3]`
- flash one example: `./blusys.sh flash examples/<name> [port] [esp32|esp32c3|esp32s3]`
- monitor one example: `./blusys.sh monitor examples/<name> [port] [esp32|esp32c3|esp32s3]`
- full build/flash/monitor: `./blusys.sh run examples/<name> [port] [esp32|esp32c3|esp32s3]`
- configure one example: `./blusys.sh config examples/<name> [esp32|esp32c3|esp32s3]`
- remove one target build dir: `./blusys.sh clean examples/<name> [esp32|esp32c3|esp32s3]`
- build every example for every target: `./blusys.sh build-examples`

High-signal script behavior:

- default target is `esp32s3` if nothing else can be inferred
- helper scripts use build dirs named `build-<target>`
- if `sdkconfig.<target>` exists, scripts pass it via `-DSDKCONFIG_DEFAULTS`
- `blusys.sh` auto-detects a serial port only when exactly one `/dev/ttyUSB*` or `/dev/ttyACM*` exists; otherwise pass the port explicitly

## Environment Gotchas

- ESP-IDF export is required before direct `idf.py` use; `blusys.sh` does it for you
- `blusys.sh` assumes:
  - export script: `/home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh`
  - default Python env: `/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.12_env`
- if ESP-IDF export fails in this repo, check missing Python packages in the IDF env; this repo previously needed `tree_sitter` and `tree_sitter_c`

## Verification Expectations

- there is no formal unit-test, lint, or formatter setup
- closest focused verification is a single smoke build, for example:
  - `./blusys.sh build examples/smoke esp32s3`
- full release-style verification is `./blusys.sh build-examples`
- hardware validation lives in `docs/guides/hardware-smoke-tests.md`
- concurrency stress examples already exist in `examples/concurrency_*`

## Docs

- docs are built with MkDocs Material via `mkdocs.yml`
- strict docs build command: `mkdocs build --strict`
- CI deploys docs from `.github/workflows/docs.yml` on pushes to `main` when docs files change

## Real Structure

- `components/blusys/include/blusys/`: full public API surface
- `components/blusys/src/common/`: module implementations
- `components/blusys/src/internal/`: private helpers for locks, timeout conversion, error translation, target caps
- `components/blusys/src/targets/esp32/`, `esp32c3/`, `esp32s3/`: target capability files
- each `examples/<name>/` is a standalone ESP-IDF app that pulls the repo component via `EXTRA_COMPONENT_DIRS`

Do not edit `esp-idf-en-v5.5.4/` unless the task is explicitly about updating the bundled upstream docs.

## Module And API Conventions

- public API is C only and uses the `blusys_` prefix
- public headers must not expose ESP-IDF types
- stateful modules use opaque handles and `open` / `close`
- GPIO is stateless and pin-based
- keep the public surface smaller than ESP-IDF; do not add options just because ESP-IDF has them

## When Adding A Module

Minimum expected pattern:

1. add `components/blusys/include/blusys/<module>.h`
2. add `components/blusys/src/common/<module>.c`
3. add target-specific code only under `src/targets/<target>/` when needed
4. register new sources and required ESP-IDF components in `components/blusys/CMakeLists.txt`
5. add a runnable example in `examples/`
6. add one task guide in `docs/guides/` and one reference page in `docs/modules/`
7. validate with at least a focused build, then broader builds if the change is shared

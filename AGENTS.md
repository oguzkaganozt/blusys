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
- `docs/guidelines.md`
- `docs/roadmap.md`

Trust scripts and CMake over prose if they disagree.

## Build And Flash Shortcuts

All commands go through the `blusys` CLI. Commands default to the current directory when no project is given:

- scaffold a project: `blusys create [path]` (default: cwd)
- build: `blusys build [project] [esp32|esp32c3|esp32s3]`
- flash: `blusys flash [project] [port] [esp32|esp32c3|esp32s3]`
- monitor: `blusys monitor [project] [port] [esp32|esp32c3|esp32s3]`
- build/flash/monitor: `blusys run [project] [port] [esp32|esp32c3|esp32s3]`
- menuconfig: `blusys config [project] [esp32|esp32c3|esp32s3]`
- firmware size: `blusys size [project] [esp32|esp32c3|esp32s3]`
- erase flash: `blusys erase [project] [port] [esp32|esp32c3|esp32s3]`
- clean one target: `blusys clean [project] [esp32|esp32c3|esp32s3]`
- clean all targets: `blusys fullclean [project]`
- build all examples: `blusys build-examples`
- version info: `blusys version`
- self-update: `blusys update`

High-signal script behavior:

- default project is the current working directory
- default target is `esp32s3` if nothing else can be inferred
- build dirs are named `build-<target>`
- if `sdkconfig.<target>` exists, it is passed via `-DSDKCONFIG_DEFAULTS`
- `blusys` auto-detects a serial port only when exactly one `/dev/ttyUSB*` or `/dev/ttyACM*` exists; otherwise pass the port explicitly

## Environment Gotchas

- `blusys` auto-detects ESP-IDF via: `$IDF_PATH` env var, `idf.py` in PATH, or scanning `~/.espressif/*/esp-idf/`
- `blusys` exports `BLUSYS_PATH` for use by project CMakeLists.txt
- if ESP-IDF detection fails, run `blusys version` to debug

## Verification Expectations

- there is no formal unit-test, lint, or formatter setup
- closest focused verification is a single smoke build, for example:
  - `blusys build examples/smoke esp32s3` (from the blusys repo directory)
- full release-style verification is `blusys build-examples`
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

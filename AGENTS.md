# AGENTS.md

## Start Here

- Read `README.md`, `PROGRESS.md`, `docs/architecture.md`, `docs/guidelines.md`, and `docs/roadmap.md` before changing public API or project status.
- Trust executable sources over prose: `blusys`, component `CMakeLists.txt`, CI workflows, and example manifests.

## Repo Facts That Matter

- This repo ships two ESP-IDF components sharing the `blusys/` header namespace:
  - `components/blusys/`: HAL only
  - `components/blusys_services/`: higher-level services; depends on `blusys`
- Supported targets are exactly `esp32`, `esp32c3`, and `esp32s3`.
- Current release tracked in repo docs is `v5.0.0`.

## Commands

- Use the `blusys` CLI, not raw `idf.py`, unless you are matching CI behavior or debugging the CLI itself.
- Most useful commands:
  - `blusys build [project] [esp32|esp32c3|esp32s3]`
  - `blusys run [project] [port] [esp32|esp32c3|esp32s3]`
  - `blusys config [project] [target]` or `blusys menuconfig [project] [target]`
  - `blusys build-examples`
  - `blusys version`
- CLI behavior that is easy to miss:
  - default target is `esp32s3`
  - build dirs are `build-<target>`
  - `sdkconfig.defaults` and `sdkconfig.<target>` are both passed through `-DSDKCONFIG_DEFAULTS`
  - serial auto-detect only works when exactly one `/dev/ttyUSB*` or `/dev/ttyACM*` exists
  - `BLUSYS_PATH` is exported by the CLI and used by generated/example `CMakeLists.txt`

## Verification

- There is no unit-test/lint/formatter pipeline to run locally.
- Fast focused verification:
  - `blusys build examples/smoke esp32s3`
  - or build the example/module you changed on all supported targets
- Broad compile gate: `blusys build-examples`
- Docs gate: `mkdocs build --strict`
- Real CI behavior from `.github/workflows/ci.yml`:
  - builds every example for all three targets
  - runs QEMU smoke tests only for `smoke`, `system_info`, `timer_basic`, `nvs_basic`, `wdt_basic`, and `concurrency_timer`
- Hardware validation is still expected for public modules; see `docs/guides/hardware-smoke-tests.md`.

## Structure

- HAL public headers: `components/blusys/include/blusys/*.h`
- HAL implementations: `components/blusys/src/common/*.c`
- HAL private helpers shared with services: `components/blusys/include/blusys/internal/`
- Target capability files: `components/blusys/src/targets/{esp32,esp32c3,esp32s3}/target_caps.c`
- Services public headers: `components/blusys_services/include/blusys/<category>/*.h`
- Services implementations: `components/blusys_services/src/<category>/*.c`
- Each `examples/<name>/` is its own ESP-IDF app using `EXTRA_COMPONENT_DIRS`.

## Conventions Agents Commonly Miss

- Public API is C only, uses the `blusys_` prefix, and must not expose ESP-IDF types.
- Keep the public surface smaller than ESP-IDF; do not wrap every backend option just because it exists.
- Stateful modules use opaque handles with `open` / `close`; GPIO is the main stateless pin API.
- Feature flags live in the HAL component:
  - add enum entries in `components/blusys/include/blusys/target.h`
  - add masks in `components/blusys/include/blusys/internal/blusys_target_caps.h`
  - update per-target `target_caps.c` when support is not universal
- Do not hold `blusys_lock_t` across blocking waits; this repo already documents that as a deadlock hazard.
- Some APIs are split across files but share one header:
  - `i2s.h` covers TX and RX, implemented in `i2s.c` and `i2s_rx.c`
  - `rmt.h` covers TX and RX, implemented in `rmt.c` and `rmt_rx.c`

## Optional Dependency Pattern

- Some modules only fully enable when the consuming project declares a managed component in `main/idf_component.yml`.
- Verified current optional integrations:
  - `usb_device` needs `espressif/esp_tinyusb`
  - `usb_hid` USB transport needs `espressif/usb_host_hid`
  - `mdns` needs `espressif/mdns`
  - `ui` needs `lvgl/lvgl`
- Follow the existing build-time detection pattern in the component `CMakeLists.txt`; do not hard-require these globally.

## When Adding Or Changing A Public Module

- Minimum expected work is not just code:
  1. header in the correct public include tree
  2. implementation in the correct component
  3. source/dependency registration in that component `CMakeLists.txt`
  4. feature flag and target-capability wiring if it is HAL-facing
  5. umbrella header update (`blusys.h` or `blusys_services.h`)
  6. runnable example in `examples/`
  7. guide in `docs/guides/`
  8. reference page in `docs/modules/`
  9. nav/index updates in `mkdocs.yml`, `docs/guides/index.md`, and `docs/modules/index.md`
  10. `PROGRESS.md`, compatibility matrix, and module inventories updated if the public surface changed

## Generated And Vendored Files

- Ignore `examples/*/build-*` and similar generated artifacts unless the task is specifically about build output.
- Do not edit `esp-idf-en-v5.5.4/` unless the task is explicitly about the bundled upstream docs.

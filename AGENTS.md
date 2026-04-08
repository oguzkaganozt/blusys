# AGENTS.md

## Start Here

- Before changing public API, docs nav, or project status, read `README.md`, `PROGRESS.md`, `docs/architecture.md`, and `docs/guidelines.md`.
- Trust executable sources over prose: `blusys`, `components/*/CMakeLists.txt`, example `idf_component.yml`, and `.github/workflows/*.yml`. Some prose inventories are older than the code.

## Repo Shape

- This repo is a library repo, not one ESP-IDF app. It ships two components sharing the `blusys/` include namespace:
  - `components/blusys/`: HAL only
  - `components/blusys_services/`: higher-level services; `REQUIRES blusys`
- Supported targets are exactly `esp32`, `esp32c3`, and `esp32s3`.
- Each `examples/<name>/` is its own ESP-IDF project. Project `CMakeLists.txt` pulls in `../../components` via `EXTRA_COMPONENT_DIRS`.
- Example/app Kconfig belongs in `main/Kconfig.projbuild`, not the project root.

## CLI And Build Quirks

- Use the `blusys` CLI for normal work; use raw `idf.py` only when matching CI behavior or debugging the CLI itself.
- Easy defaults to guess wrong:
  - project defaults to the current directory
  - default target is `esp32s3`
  - build dirs are `build-<target>`
  - both `sdkconfig.defaults` and `sdkconfig.<target>` are passed through `-DSDKCONFIG_DEFAULTS`
  - serial auto-detect only works when exactly one `/dev/ttyUSB*` or `/dev/ttyACM*` exists
  - generated projects and examples rely on `BLUSYS_PATH`, which the CLI exports
- If you change Kconfig or `sdkconfig.defaults`, remove `build-<target>` before rebuilding; `set-target` only runs on first configure.
- Useful commands:
  - `blusys build [project] [esp32|esp32c3|esp32s3]`
  - `blusys run [project] [port] [esp32|esp32c3|esp32s3]`
  - `blusys config [project] [target]` or `blusys menuconfig [project] [target]`
  - `blusys build-examples`
  - `blusys qemu [project] [target]` after `blusys install-qemu`; QEMU networking is Ethernet emulation, not WiFi

## Verification

- There is no repo-local unit/lint/typecheck pipeline.
- Fast compile check: `blusys build examples/smoke esp32s3` or build the changed example/module on all supported targets.
- Broad compile gate: `blusys build-examples`.
- Docs gate: `mkdocs build --strict`.
- CI builds every example for all three targets. QEMU only covers `smoke`, `system_info`, `timer_basic`, `nvs_basic`, `wdt_basic`, and `concurrency_timer`.
- Public modules still need real-board validation; see `docs/guides/hardware-smoke-tests.md`.

## API And Implementation Rules

- Public API is C-only, `blusys_`-prefixed, and must not expose ESP-IDF types or `esp_err_t`.
- Stateful modules use opaque handles with `open` / `close`; GPIO is the main stateless pin API.
- Keep the public surface smaller than ESP-IDF; do not mirror backend options by default.
- Shared internal helpers live in `components/blusys/include/blusys/internal/`.
- Modules that need NVS should use `blusys_nvs_ensure_init()` instead of open-coding `nvs_flash_init()` / erase-retry.
- Never hold `blusys_lock_t` across blocking waits; the expected pattern is lock -> prepare -> unlock -> wait.
- `i2s.h` and `rmt.h` each cover both TX and RX, but the implementations are split across `i2s.c` / `i2s_rx.c` and `rmt.c` / `rmt_rx.c`.

## Feature Gating

- HAL-facing feature flags live only in `components/blusys`: update `include/blusys/target.h`, `include/blusys/internal/blusys_target_caps.h`, and per-target `src/targets/*/target_caps.c` when support is not universal.
- Follow the existing optional managed-component detection pattern in CMake instead of hard-requiring these globally:
  - `usb_device` -> `espressif/esp_tinyusb`
  - `mdns` -> `espressif/mdns`
  - `usb_hid` USB transport -> `espressif/usb_host_hid`
  - `ui` -> `lvgl/lvgl`
- `bluetooth` and `ble_gatt` require `CONFIG_BT_NIMBLE_ENABLED`.
- WiFi-dependent service modules do not bring up connectivity for you. Verified docs require WiFi to be connected first for `http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, and `local_ctrl`.

## Public Module Changes

- Minimum expected work for a new or changed public module is more than code:
  - header in the correct public include tree
  - implementation in the correct component and source registration in that component's `CMakeLists.txt`
  - umbrella header update (`blusys.h` or `blusys_services.h`)
  - runnable example in `examples/`
  - guide in `docs/guides/` and reference page in `docs/modules/`
  - nav updates in `mkdocs.yml` and card-index updates in `docs/guides/index.md` and `docs/modules/index.md`
  - `PROGRESS.md` and `docs/target-matrix.md` if the public surface or support matrix changed
- For HAL modules only, also wire feature flags and target capability masks.

## Generated Files

- Ignore generated artifacts under `examples/**/build*`, `examples/**/sdkconfig*` except `sdkconfig.defaults*`, and `site/` unless the task is specifically about generated output.
- Do not edit vendored upstream docs under `esp-idf-en-*` unless the task is explicitly about that copy.

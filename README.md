# Blusys HAL

Blusys HAL is a simplified C HAL for ESP32 devices built on top of ESP-IDF v5.5.

The first release targets:
- ESP32-C3
- ESP32-S3

Project goals:
- simpler than ESP-IDF for common embedded tasks
- modular and maintainable internally
- stable public API and documentation
- common API surface across C3 and S3 for v1

This repository contains the project planning baseline and the implemented Phase 1 and Phase 2 modules.

Current project status:
- roadmap and project rules are documented
- Phase 1 is completed
- Phase 2 is completed
- the `blusys` ESP-IDF component exists
- public `system` and `gpio` modules exist
- the `smoke`, `system_info`, and `gpio_basic` examples build for both `esp32c3` and `esp32s3`

Track progress in:
- `PROGRESS.md`

Primary project documents:
- `PROGRESS.md`
- `docs/index.md`
- `docs/vision.md`
- `docs/architecture.md`
- `docs/api-design-rules.md`
- `docs/target-matrix.md`
- `docs/roadmap.md`
- `docs/testing-strategy.md`
- `docs/development-workflow.md`
- `docs/documentation-standards.md`
- `docs/modules/v1-core.md`

Bundled upstream reference documentation:
- `esp-idf-en-v5.5.4/`

Current implementation layout:
- `components/blusys/` for the ESP-IDF component
- `examples/smoke/` for general build validation
- `examples/system_info/` for the `system` module example
- `examples/gpio_basic/` for the `gpio` module example

Phase 1 foundation currently includes:
- `version` public API
- `error` public API
- `target` public API
- internal target capability plumbing for `esp32c3` and `esp32s3`
- shared internal lock abstraction for future handle-based modules

Phase 2 foundational public modules currently include:
- `system` public API
- `gpio` public API
- `system_info` example app
- `gpio_basic` example app
- task-first guides and API reference pages for `system` and `gpio`

Recommended local docs workflow:
- install docs dependencies with `pip install -r requirements-docs.txt`
- use `mkdocs serve` for preview
- use `mkdocs build` for static output

Recommended validation workflow:
- if `export.sh` looks for a missing `idf5.5_py3.14_env`, first set `IDF_PYTHON_ENV_PATH=/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.10_env`
- source ESP-IDF 5.5.4 with `source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh`
- build `examples/smoke/` for `esp32c3` with `-DSDKCONFIG=sdkconfig.esp32c3`
- build `examples/smoke/` for `esp32s3` with `-DSDKCONFIG=sdkconfig.esp32s3`
- build `examples/system_info/` for both targets
- build `examples/gpio_basic/` for both targets

Remote VPS validation note: commit/push test performed from this machine.

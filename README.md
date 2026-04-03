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

This repository contains the project planning baseline and the Phase 1 implementation scaffold.

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
- `examples/smoke/` for the Phase 1 build-validation app

Recommended local docs workflow:
- install docs dependencies with `pip install -r requirements-docs.txt`
- use `mkdocs serve` for preview
- use `mkdocs build` for static output

Recommended Phase 1 validation workflow:
- source ESP-IDF 5.5.4 with `source /home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh`
- build `examples/smoke/` for `esp32c3` with `-DSDKCONFIG=sdkconfig.esp32c3`
- build `examples/smoke/` for `esp32s3` with `-DSDKCONFIG=sdkconfig.esp32s3`

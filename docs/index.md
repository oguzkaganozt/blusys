# Blusys HAL

Blusys HAL is a simplified C hardware abstraction layer for ESP32 devices using ESP-IDF v5.5.

The first release supports:
- ESP32-C3
- ESP32-S3

The project exists to make normal embedded development tasks easier than working directly against raw ESP-IDF APIs and documentation.

## Principles

- keep the public API smaller than ESP-IDF
- keep the public API stable and easy to read
- expose only the common C3 and S3 subset in v1
- hide ESP-IDF and target-specific complexity internally
- ship task-first documentation alongside each module
- design for thread safety from the beginning

## Reading Order

1. `vision.md`
2. `architecture.md`
3. `api-design-rules.md`
4. `target-matrix.md`
5. `roadmap.md`
6. `modules/v1-core.md`

## Project Status

This documentation set defines the project contract before implementation starts.

## Upstream Reference

The bundled ESP-IDF v5.5 documentation lives in `../esp-idf-en-v5.5.4/` and remains the upstream reference when deeper backend behavior must be verified.

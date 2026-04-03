# Vision

## Mission

Blusys HAL provides a small, direct, well-documented C API for common ESP32 peripheral tasks so users can solve normal product needs without living inside ESP-IDF documentation.

## Product Goals

- simplify common hardware tasks
- reduce setup and integration friction
- keep implementation modular and maintainable
- keep documentation focused on tasks first and reference second
- support ESP32-C3 and ESP32-S3 through one common v1 API

## Success Criteria

- a new user can complete common tasks by reading Blusys docs first
- public headers are smaller and easier to scan than the corresponding ESP-IDF APIs
- examples build for both C3 and S3
- common concurrency behavior is documented and testable
- the v1 API does not require target-specific `#ifdef` usage for normal cases

## Non-Goals For V1

- networking abstraction
- BLE abstraction
- board support helpers
- wrapping every ESP-IDF feature
- exposing internal ESP-IDF HAL or LL concepts publicly

## V1 Platform Promise

Blusys v1 supports the common subset of ESP32-C3 and ESP32-S3 peripherals. Chip-specific features stay internal or are deferred to later extension modules.

# Vision

Blusys HAL exists to make common ESP32 peripheral work simpler than using raw ESP-IDF directly.

## Goals

- keep the public API small and readable
- support one shared API across `esp32`, `esp32c3`, and `esp32s3`
- keep docs task-first and reference pages short
- hide target-specific and ESP-IDF-specific details internally

## Success Means

- users can complete common tasks from Blusys docs first
- examples build on all supported targets
- normal application code does not need target-specific `#ifdef` logic

## Non-Goals For V1

- networking or BLE abstraction
- board support helpers inside the core HAL
- wrapping every ESP-IDF feature
- exposing internal HAL or LL details publicly

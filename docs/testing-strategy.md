# Testing Strategy

## Objectives

- verify common behavior across ESP32-C3 and ESP32-S3
- verify lifecycle correctness and error handling
- verify thread-safety assumptions under task concurrency
- keep examples usable as smoke tests and documentation assets

## Test Layers

### 1. Host-Side Unit Tests

Use for:
- error mapping
- argument validation
- small state machines
- helper utilities that do not require hardware

### 2. Build Matrix Validation

Every example and test app should build for:
- `esp32c3`
- `esp32s3`

Use target-specific defaults when needed:
- `sdkconfig.defaults`
- `sdkconfig.defaults.esp32c3`
- `sdkconfig.defaults.esp32s3`

### 3. Hardware Smoke Tests

Required on real boards for:
- gpio
- uart
- i2c
- spi
- pwm
- adc
- timer

### 4. Concurrency And Stress Tests

Required for:
- shared handle access rules
- lifecycle edge cases such as close during active use
- callback behavior
- timeout behavior

## Module Definition Of Done

Each module is only complete when all items below are true:
- public header exists
- implementation exists
- task-first guide exists
- example exists
- both targets build successfully
- at least one hardware smoke test passed
- thread-safety note is documented
- major failure paths are documented

## Thread-Safety Validation

Special focus areas:
- concurrent access to shared I2C bus
- concurrent access to the same SPI handle
- timer start and stop races
- UART async and blocking interaction
- safe shutdown and callback deregistration behavior

## Test Philosophy

- test the public API, not internal ESP-IDF details
- keep real hardware tests small and deterministic
- add regression tests when bugs are fixed

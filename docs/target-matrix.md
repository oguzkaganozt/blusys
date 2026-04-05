# Compatibility

## Supported Targets

Blusys supports these targets:

- `esp32`
- `esp32c3`
- `esp32s3`

## Common Library Surface

The shared API currently includes:

- `system`
- `target`
- `gpio`
- `uart`
- `i2c`
- `i2s`
- `spi`
- `pwm`
- `adc`
- `timer`
- `pcnt`
- `rmt`
- `twai`
- `touch`

## Compatibility Rules

- the public API is the same across supported targets
- application code should not need target-specific `#ifdef` logic for normal use
- target-specific implementation details stay internal to the library

## Practical Notes

- some examples use board-dependent pin defaults, so check `menuconfig` when needed
- runtime behavior can still differ because boards expose different safe pins and wiring constraints
- thread-safety rules are defined in task terms, not CPU-core terms
- `pcnt` is available on `esp32` and `esp32s3`; `esp32c3` reports `BLUSYS_FEATURE_PCNT` as unsupported in the current ESP-IDF baseline
- `rmt` TX is available on `esp32`, `esp32c3`, and `esp32s3`
- `twai` classic TX and RX callback support is available on `esp32`, `esp32c3`, and `esp32s3`
- `i2s` standard-mode master TX is available on `esp32`, `esp32c3`, and `esp32s3`
- `touch` polling support is available on `esp32` and `esp32s3`; `esp32c3` reports `BLUSYS_FEATURE_TOUCH` as unsupported

## Out Of Scope For V1

These are intentionally not part of the common public surface:

- advanced I2S feature sets beyond the shipped standard TX subset
- touch sensor support
- DAC
- LP peripheral variants
- storage-specific abstractions
- board support package behavior inside the core HAL

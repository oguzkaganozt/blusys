# Compatibility

## Supported Targets

Blusys supports these targets in v1:

- `esp32`
- `esp32c3`
- `esp32s3`

## Common Library Surface

The shared v1 API includes:

- `system`
- `target`
- `gpio`
- `uart`
- `i2c`
- `spi`
- `pwm`
- `adc`
- `timer`

## Compatibility Rules

- the public API is the same across supported targets
- application code should not need target-specific `#ifdef` logic for normal use
- target-specific implementation details stay internal to the library

## Practical Notes

- some examples use board-dependent pin defaults, so check `menuconfig` when needed
- runtime behavior can still differ because boards expose different safe pins and wiring constraints
- thread-safety rules are defined in task terms, not CPU-core terms

## Out Of Scope For V1

These are intentionally not part of the common public surface:

- advanced I2S feature sets
- touch sensor support
- DAC
- LP peripheral variants
- storage-specific abstractions
- board support package behavior inside the core HAL

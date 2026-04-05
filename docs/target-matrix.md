# Target Matrix

## Supported Targets For V1

| Target | Status | Notes |
|---|---|---|
| ESP32 | supported | dual-core; full v1 API surface |
| ESP32-C3 | primary | part of the common v1 API baseline |
| ESP32-S3 | primary | part of the common v1 API baseline |

## Common V1 Surface

The common public API is built around peripherals and behaviors that are practical on both ESP32-C3 and ESP32-S3.

Included in the common v1 surface:
- system
- target information
- gpio
- uart
- i2c master
- spi master
- pwm
- adc oneshot with calibration
- timer

## Deferred Or Internal Differences

Keep these out of the common public v1 API:
- advanced I2S feature differences
- touch sensor support
- DAC
- LP peripheral variants
- storage and SD-specific abstractions
- S3-specific feature surfaces

## Runtime And Concurrency Considerations

- ESP32 is dual-core (2× Xtensa LX6)
- ESP32-C3 is single-core
- ESP32-S3 may run in SMP or unicore configurations

Implication for Blusys:
- the public API must not depend on explicit core-affinity concepts
- thread safety must be defined in task-oriented terms, not CPU-core terms

## Extension Policy

If later modules need chip-specific behavior, prefer one of these:
- internal capability handling with unchanged public API
- a new optional extension module

Do not widen the base API with target-specific branches unless there is a strong product reason.

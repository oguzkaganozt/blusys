# Roadmap

Short version: `v1.0.0` is released, `V2` is in progress, and detailed tracking lives in `../PROGRESS.md`.

## Status

- `v1.0.0` released
- current work: `V2`
- current milestone: `rmt`

## Completed

- foundation and project setup
- core modules: `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer`
- async support and validation work
- `pcnt` started as the first `V2` item

## V2

Core HAL expansion.

- done: `pcnt`
- next: `rmt`
- remaining: `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `mcpwm`, `rtc` / low-power / sleep-control, temperature sensor, `wdt`

## V3

Connectivity and system services.

- `usb`
- `wifi`
- `bluetooth`
- `eth`
- `nvs`
- `ota`

## V4

Advanced and ecosystem-level helpers.

- `efuse`
- `ulp`
- advanced power helpers
- BSP layer
- diagnostics and self-test helpers
- security and provisioning helpers
- other higher-level service wrappers

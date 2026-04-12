# Internals

Platform architecture, development guidelines, display notes, testing procedures, and other engineering references.

## Core

- [Architecture](architecture.md) --- three-tier model, component dependencies, design rationale
- [Guidelines](guidelines.md) --- API design rules, development workflow, documentation standards
- [Target Matrix](target-matrix.md) --- per-module target support and requirements

## Display & integration

- [LVGL integration (ST7735, resolved)](lvgl-st7735-integration.md) --- root-cause notes for SPI DMA + LVGL stride on ESP32-C3

## Testing

- [Hardware Smoke Tests](testing/hardware-smoke-tests.md) --- manual hardware validation procedures
- [Concurrency Tests](testing/concurrency-tests.md) --- concurrent lifecycle and locking validation
- [Validation Report Template](testing/hardware-validation-report-template.md) --- template for hardware validation reports

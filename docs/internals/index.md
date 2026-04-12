# Internals

Platform architecture, development guidelines, display notes, testing procedures, and other engineering references.

## Core

- [Architecture](architecture.md) --- three-tier model, component dependencies, design rationale
- [Maintaining](maintaining.md) --- component names, suggested reading order, pre-merge checks
- [Guidelines](guidelines.md) --- API design rules, development workflow, documentation standards
- [UI layout and LVGL flex](ui-layout-lvgl.md) --- flex/scroll usage in the framework, shell, and stock widgets
- [Target Matrix](target-matrix.md) --- per-module target support and requirements
- [ADR 0001 — orchestration in framework](adr/0001-orchestration-in-framework.md) --- tier roles and non-goals for product orchestration
- [ADR 0002 — HAL component name `blusys_hal`](adr/0002-hal-component-name.md) --- ESP-IDF component folder vs `blusys/` include namespace
- [Integration baseline](integration-baseline.md) --- measuring `main/integration/` thickness (V2 metrics)

## Display & integration

- [LVGL integration (ST7735, resolved)](lvgl-st7735-integration.md) --- root-cause notes for SPI DMA + LVGL stride on ESP32-C3

## Testing

- [Hardware Smoke Tests](testing/hardware-smoke-tests.md) --- manual hardware validation procedures
- [Concurrency Tests](testing/concurrency-tests.md) --- concurrent lifecycle and locking validation
- [Validation Report Template](testing/hardware-validation-report-template.md) --- template for hardware validation reports

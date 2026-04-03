# Roadmap

## Phase 0: Project Freeze

Deliverables:
- architecture baseline
- API design rules
- target matrix
- roadmap and testing strategy
- docs site skeleton

Exit criteria:
- the project contract is written down
- implementation can start without re-opening core direction questions

## Phase 1: Foundation

Deliverables:
- repository skeleton
- base build layout
- `version`, `error`, `target`, and shared internal utilities
- internal lock abstraction
- target capability plumbing for C3 and S3

Exit criteria:
- common infrastructure exists for all modules

## Phase 2: Foundational Public Modules

Deliverables:
- `system`
- `gpio`
- first examples
- first task-first guides

Exit criteria:
- users can build and run the simplest Blusys examples on both targets

## Phase 3: Core Serial And Bus Modules

Deliverables:
- `uart`
- `i2c` master
- `spi` master
- examples and docs for each module

Exit criteria:
- common communication peripherals work on both targets

## Phase 4: Timing And Analog Modules

Deliverables:
- `pwm`
- `adc`
- `timer`
- examples and docs for each module

Exit criteria:
- full planned v1 blocking module set exists

## Phase 5: Async And Hardening

Deliverables:
- GPIO interrupt callback surface
- UART async TX and RX
- timer callback behavior
- lifecycle and concurrency hardening

Exit criteria:
- minimum v1 async support is stable and documented

## Phase 6: Stabilization

Deliverables:
- cross-target smoke tests
- concurrency tests
- documentation cleanup
- API review and freeze candidate

Exit criteria:
- release candidate quality reached

## Phase 7: Release

Deliverables:
- `v1.0.0` release candidate
- changelog
- migration notes if needed
- release validation

Exit criteria:
- first stable public release

## Suggested Version Milestones

| Version | Meaning |
|---|---|
| `v0.1.0` | foundation plus first public modules |
| `v0.5.0` | full blocking v1 module set |
| `v0.8.0` | minimum async support added |
| `v0.9.0-rc1` | release candidate |
| `v1.0.0` | stable first release |

## Post-V1 Candidates

- `rmt`
- `twai`
- selected `i2s` subset if justified by product needs
- service-layer libraries outside core HAL
- board support package as a separate layer, not inside core HAL

# Progress

## Current Summary

- current track: `Platform transition`
- current phase: `Phase 5/6 prep — framework core implementation`
- target release: `v6.0.0`
- overall status: `in_progress`

Blusys is mid-transition from a HAL/services library repo into an internal embedded product platform. The planning source of truth lives under [`platform-transition/`](platform-transition/).

- roadmap: [`platform-transition/ROADMAP.md`](platform-transition/ROADMAP.md)
- decisions: [`platform-transition/DECISIONS.md`](platform-transition/DECISIONS.md)
- architecture: [`platform-transition/application-layer-architecture.md`](platform-transition/application-layer-architecture.md)
- C++ policy: [`platform-transition/CPP-transition.md`](platform-transition/CPP-transition.md)
- UI system: [`platform-transition/UI-system.md`](platform-transition/UI-system.md)

## Transition Status

| # | Phase | Status |
|---|---|---|
| 1 | Lock platform positioning | completed |
| 2 | Identity alignment | completed |
| 3 | Packaging shape | completed |
| 4 | C++ infrastructure (framework only) + `blusys/log.h` | completed |
| 4.5 | PC + SDL2 host harness | pending |
| 5 | Flagship widget and V1 widget kit | pending |
| 6 | Framework core V1 | pending |
| 7 | Product scaffold and sample apps | pending |
| 8 | Example ecosystem migration | pending |
| 9 | Validation and migration notes | pending |

## Locked Direction

- three tiers in V1: `blusys` (HAL + drivers, C) -> `blusys_services` (C) -> `blusys_framework` (C++)
- framework is the only C++ tier in V1; services-to-C++ migration is deferred to V2
- drivers move under `components/blusys/src/drivers/` and are separated from HAL by directory rules + CI lint
- UI is optional and gated by `BLUSYS_BUILD_UI`, derived from `BLUSYS_STARTER_TYPE`
- product repos consume the platform through managed components pinned to one repo tag per release

## Active Review Findings

No open issues. The planning docs have gone through three review passes and all flagged items are resolved:

- Pass 1: C++20 baseline, component-granular build, single-version pinning, semantic callback types.
- Pass 2: managed-components vs `EXTRA_COMPONENT_DIRS` consumption model unified; `app/` scaffold compilation specified via its own ESP-IDF component; Camp 3 widget template fixed to call `lv_obj_class_init_obj`; bootstrap unified to `main/app_main.cpp` for both profiles; widget naming normalized.
- Pass 3: sample screen include path corrected to match `INCLUDE_DIRS "."` convention; `ui/widgets` added to the scaffold template; product-facing authoring surface reframed to distinguish `product_config.cmake` from scaffold-generated files (including the conditional `lvgl/lvgl` managed dep).

## Landed Implementation

- repo/docs identity updated from HAL-only wording to platform transition wording
- driver modules moved into `components/blusys/src/drivers/` with public headers under `components/blusys/include/blusys/drivers/`
- `usb_hid` kept in `blusys_services` and build-fixed for current `usb_host_hid` APIs
- `blusys lint` added to enforce the HAL/drivers boundary
- example includes and `REQUIRES` entries updated for the driver move
- `components/blusys_framework/` added as a real component with C++20 compile policy
- foundational framework files added: `framework.hpp`, `core/containers.hpp`, `framework.cpp`, `README.md`
- `blusys/log.h` added for framework-side logging
- docs/nav updated to reflect HAL + Drivers + Services + Framework structure

## Verification Snapshot

- `./blusys lint` passes
- `./blusys build examples/button_basic esp32s3` passes
- `./blusys build examples/ui_basic esp32s3` passes
- `./blusys build examples/usb_hid_basic esp32s3` passes
- `mkdocs build --strict` passes

## Immediate Next Actions

1. Implement real `blusys_framework/core` APIs: router, intent, feedback, controller lifecycle.
2. Decide whether Phase 5 widget work or Phase 6 core work should lead once the first framework contracts are in place.
3. Add the first framework-consuming example once the core surface is stable.

# ADR 0002: HAL ESP-IDF component name `blusys_hal`

## Status

Accepted

## Context

The repository ships three ESP-IDF components under `components/`. The HAL + drivers tier lived in a folder named `blusys/`, which was easy to confuse with the whole platform or with the shared **`blusys/`** public include prefix used by all three components.

## Decision

Rename the HAL component directory to **`components/blusys_hal/`**. The ESP-IDF component name (used in `REQUIRES`) is **`blusys_hal`**.

Public C/C++ includes remain **`#include "blusys/..."`** — unchanged on purpose so product code and documentation about the header namespace stay stable.

## Consequences

- Example and scaffold `CMakeLists.txt` files must use `REQUIRES blusys_hal` for HAL-only dependencies.  
- Docs and scripts that referenced `components/blusys/` for the HAL tree must say `components/blusys_hal/`.  
- No change to the layering model: `blusys_framework` → `blusys_services` → `blusys_hal`.

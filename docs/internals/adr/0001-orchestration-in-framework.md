# ADR 0001: Orchestration in framework; mechanical services; peripheral-shaped HAL

**Status:** Accepted (partially superseded by v0 platform restructure — see architecture.md)  
**Date:** 2026-04-12  
**Context:** Product-platform evolution (thinner `main/platform/`, clearer tier roles).

## Decision

1. **Recurring product orchestration** (connected bootstrap, provisioning + Wi‑Fi + time + safe OTA gates, navigation policy) belongs in **`blusys_framework`** as typed APIs, **flows**, and capability composition—not duplicated per product in `platform/`.
2. **`blusys_services`** remains the **mechanical** runtime tier: ESP-IDF integration, opaque handles, explicit lifecycle, minimal product policy. Services stay **C** unless a future ADR explicitly changes language policy.
3. **`blusys` (HAL + drivers)** stays **peripheral- and bus-shaped**. Connectivity stacks (Wi‑Fi, BT, MQTT, etc.) are **not** “HAL drivers” in this model; they remain **services** built on HAL + ESP-IDF.
4. **Dependency direction is unchanged:** `blusys_framework` → `blusys_services` → `blusys_hal`. No tier collapse.

## Consequences

- FFI pressure at the framework/services boundary is reduced by **narrower public orchestration APIs** and **internal C++ facades** over C services, not by moving product logic into `blusys_services`.
- HAL growth stays gated by **inventory**, layering lint, and “driver-shaped only” moves downward from services.

## Non-goals

- Rewriting the entire services tier in C++ as a blanket migration.
- Merging Wi‑Fi/BT stack glue into HAL to pretend there are only two layers.

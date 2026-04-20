# AGENTS.md

## Source Of Truth
- Trust `inventory.yml`, `scripts/*`, and `.github/workflows/*.yml` over prose; `scripts/check-inventory.py` and `scripts/check-product-layout.py` are the example/layout gates.
- Example CI is inventory-driven: `ci_pr: true` is the PR build set, `ci_nightly: true` is nightly, and `layout_exempt` / `product_layout` control the product-layout check.
- New public API should come with matching examples, docs, and validation; keep the public surface smaller than the backend surface.

## Repo Shape
- `components/blusys/` is the single framework component.
- Inside `components/blusys/`, the layer order is `hal -> drivers -> services -> framework`; keep includes one-way.
- `examples/quickstart/` are the canonical starters, `examples/reference/` are deeper demos, and `examples/validation/` are internal smoke apps.
- Product apps use `main/core`, `main/ui`, and `main/platform`; `main/platform/app_main.cpp` is the entrypoint and the only downward bridge.
- Keep `core` and `ui` framework-only; product logic should stay reducer-first (`update(ctx, state, action)` in-place), use `ctx.fx()` for navigation/overlays, and keep raw LVGL out of product code except explicit custom widget/view scope.
- Examples and product code should include only `<blusys/blusys.hpp>`; use `<blusys/blusys.h>` only in C HAL/driver/service code.

## Commands
- `blusys build . esp32|esp32c3|esp32s3` builds firmware, `blusys host-build` builds the host harness, and `blusys qemu . qemu32s3 [--graphics|--serial-only]` runs QEMU.
- `blusys build` targets `build-esp32*`, host builds land in `scripts/host/build-host` here, and QEMU builds use `build-qemu*`.
- `blusys flash`, `run`, `monitor`, and `erase` are device-target only.
- `blusys lint` runs `scripts/lint-layering.sh`, `scripts/check-framework-ui-sources.py`, and `scripts/check-host-bridge-spine.py`.
- `blusys validate` runs inventory, product-layout, and lint checks in one pass.
- `blusys example <name> ...` runs a command on a bundled example; `blusys build-examples` builds the full example matrix.
- `blusys build-inventory <target> <ci_pr|ci_nightly>` is the inventory-filtered example-build entry point.
- `scripts/host/` needs SDL2, `pkg-config`, and `libmosquitto-dev`; `scripts/host-test/` is the minimal headless loop with no SDL/LVGL/mosquitto dependency.

## Checks
- Run `python3 scripts/check-inventory.py` before `python3 scripts/check-product-layout.py`.
- For focused host tests, use `ctest --test-dir build-host-test --output-on-failure -R host_harness_smoke` or `session_helper_smoke`.
- `blusys build-inventory <target> ci_pr` mirrors PR CI; use `ci_nightly` for nightly-only example coverage.
- `./scripts/scaffold-smoke.sh` checks the scaffold matrix and host-build flow.
- `mkdocs build --strict` and `doxygen api-docs/Doxyfile` are the strict docs/API checks.
- The launcher can reuse `IDF_PATH` and `QEMU_PATH` from `~/.config/blusys/config` if environment variables are unset.

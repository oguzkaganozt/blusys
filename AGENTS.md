# AGENTS.md

## Source Of Truth
- Trust `inventory.yml`, `scripts/*`, and `.github/workflows/*.yml` over prose; if docs conflict with executable sources, follow the executable source.
- `blusys validate` and `blusys build-inventory` require PyYAML (`pip install pyyaml`).
- Do not commit `build/generated/**`; `blusys validate` rejects tracked `build/generated/**` paths and the repo hook runs it on staged manifests/generated paths.

## Layout
- `components/blusys/` is the only component.
- Layer order is `hal -> drivers -> services -> framework`; the only downward exceptions are `framework/platform/`, device-only bridges in `framework/capabilities/` under `#ifdef ESP_PLATFORM`, and framework-wide `blusys/hal/log.h` / `blusys/hal/error.h`.
- Product code lives in `main/core`, `main/ui`, and `main/platform`; `main/platform/app_main.cpp` is the entrypoint and the only product-level downward bridge.
- Keep `core` and `ui` framework-only. Product logic stays reducer-first (`update(ctx, state, action)` in place) and uses `ctx.fx()` for navigation/overlays. Avoid raw LVGL outside explicit custom widget/view scope.
- Product and example code should include only `<blusys/blusys.hpp>`; use `<blusys/blusys.h>` only in C HAL/driver/service/observe code.

## Examples
- `examples/reference/` are public demos; `examples/validation/` are internal smokes.
- `inventory.yml` drives example selection and layout checks. `ci_pr: true` is the PR build set, `ci_nightly: true` is nightly.
- `layout_exempt: true` skips product-layout; `product_layout: true` forces it on validation-style apps.
- Run `python3 scripts/check-inventory.py` before `python3 scripts/check-product-layout.py`.

## Commands
- `blusys create` is the scaffold command; it writes top-level CMake, `main/`, `host/`, `sdkconfig` defaults, and `blusys.project.yml`.
- `blusys validate` is the main preflight. It always runs the generated-artifact check first; with no path and no local `blusys.project.yml`, it does the full repo sweep (inventory, product-layout, `scripts/layer-invariants.sh`, framework UI + host-bridge checks, and manifest scans). With manifest path(s) or a local manifest, it only schema-validates those manifests after that check.
- `blusys lint` runs `scripts/lint-layering.sh`, `scripts/check-framework-ui-sources.py`, and `scripts/check-host-bridge-spine.py`.
- `blusys build-inventory <target> <ci_pr|ci_nightly>` mirrors CI example selection; empty `ci_pr` fails unless `BLUSYS_ALLOW_EMPTY_CI_PR=1`.
- `blusys build . host` is the same as `blusys host-build`. Without a project path, `host-build` uses a local `host/` dir if present, otherwise `scripts/host/`.
- `blusys build . esp32|esp32c3|esp32s3` writes `build-esp32*`; `qemu32*` writes `build-qemu*` so QEMU `sdkconfig` does not collide with chip builds.
- `blusys qemu` defaults to UART `-nographic`; use `--graphics` only for framebuffer apps and `--serial-only` when you want monitor-only output.
- `blusys flash`, `blusys run`, `blusys monitor`, and `blusys erase` are device-only.
- `blusys gen-spec` regenerates `build/generated/blusys_app_spec.h` from `blusys.project.yml`.

## Host And CI
- `scripts/host/` needs `cmake`, `pkg-config`, SDL2, and `libmosquitto-dev`; `scripts/host-test/` is headless and has no SDL/LVGL/mosquitto dependency.
- `BLUSYS_HOST_ZOOM=1` forces 1:1 SDL pixels for host UI debugging.
- Focused host-test runs are `ctest --test-dir build-host-test --output-on-failure -R host_harness_smoke` and `... -R session_helper_smoke`.
- `scripts/scaffold-smoke.sh` is the scaffold matrix check and is part of CI.
- `mkdocs build --strict` and `doxygen api-docs/Doxyfile` are the strict docs/API checks.

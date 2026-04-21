# Blusys host harness — PC + SDL2 + LVGL

Builds the SDL2/LVGL host suite on Linux so interactive demos and host smokes can iterate without flashing hardware on every change. The LVGL version is pinned to the same upstream tag as the ESP-IDF managed component (`lvgl/lvgl ~9.2`, currently resolving to **v9.2.2** — see `examples/framework_*/dependencies.lock`).

Two host-side CMake projects matter for this flow:

- **`scripts/host/`** — SDL2 + LVGL interactive demos and host smokes.
- **`scripts/host-test/`** — the minimal headless loop with no SDL, no LVGL window, and no libmosquitto dependency.

The interactive starters are still `hello_lvgl` and `widget_kit_demo`.

## Product example host builds

Example `host/CMakeLists.txt` files include `cmake/blusys_host_bridge.cmake` via `include("$ENV{BLUSYS_PATH}/cmake/blusys_host_bridge.cmake")` after exporting `BLUSYS_PATH` (the `blusys` CLI sets this for `blusys host-build`). The bridge sets the `BLUSYS_PATH` CMake variable from the environment when needed.

## Prerequisites

The host build needs SDL2 development headers, `cmake >= 3.16`, `pkg-config`, and a C/C++ toolchain.

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install build-essential cmake pkg-config libsdl2-dev
```

### Fedora / RHEL

```bash
sudo dnf install gcc gcc-c++ cmake pkgconf-pkg-config SDL2-devel
```

### Arch / Manjaro

```bash
sudo pacman -S base-devel cmake pkgconf sdl2
```

### macOS

```bash
brew install cmake pkg-config sdl2
```

## Additional validation targets

These ship with the host harness and do not require hardware:

| Binary | Purpose |
|--------|---------|
| `capability_contract_smoke` | Static checks that capability integration event IDs stay in their reserved bands |
| `interactive_starter_reducer_smoke` | Exercises `blusys_examples::clamp_percent` shared with interactive quickstart reducers |
| `panel_connectivity_map_smoke` | `panel_connectivity_event_triggers_sync` parity with the interactive panel example's `on_event` bridge |
| `operational_phase_smoke` | Runtime checks for the headless-telemetry-style operational phase state machine |
| `connected_headless_host` | Headless `blusys::app` loop with connectivity + storage host stubs and a broad `on_event` bridge |

## Build and run

The wrapper goes through the `blusys` CLI:

```bash
blusys host-build                                  # configure + build
./scripts/host/build-host/hello_lvgl               # LVGL-only smoke test
./scripts/host/build-host/widget_kit_demo          # framework widget kit demo
./scripts/host/build-host/interactive_starter_reducer_smoke
./scripts/host/build-host/panel_connectivity_map_smoke
./scripts/host/build-host/operational_phase_smoke  # operational phase-machine smoke
./scripts/host/build-host/connected_headless_host  # headless capability + reducer smoke
```

`hello_lvgl` uses a 480×320 **logical** LVGL resolution (see `hello_lvgl.c`); the SDL surface is scaled automatically for visibility. Closing the window (or pressing the SDL window close button) exits via `LV_SDL_DIRECT_EXIT`.

`widget_kit_demo` opens a similar window, but the screen is built entirely through the framework widget kit: a title + divider + volume slider + three buttons (`Down` / `Up` / `OK`). Clicking any of the buttons posts a semantic intent to the runtime; the `Down` / `Up` buttons mutate the slider, and `OK` submits a `show_overlay` route command that pops a transient toast. Feedback events are logged to the host console. The mouse acts as the "encoder": clicks drive the runtime exactly the way a rotary encoder press would on hardware.

If you'd rather drive CMake directly:

```bash
cd scripts/host
cmake -B build-host -S .
cmake --build build-host -j
./build-host/hello_lvgl
```

The first configure pulls LVGL from `https://github.com/lvgl/lvgl.git` at the pinned tag via CMake `FetchContent` and caches it under `build-host/_deps/`. Subsequent builds reuse the cache.

### Host window scaling (interactive `blusys::app` builds)

`scripts/host/src/app_host_platform.cpp` creates the SDL display at the **logical** width/height from the product `host_profile` (same coordinate space as the matching device profile). It then applies an **automatic integer zoom** (via LVGL’s `lv_sdl_window_set_zoom`) so the **physical** SDL window is comfortable to use—typically scaling the short edge toward ~640 px—**without** changing layout math or asking you to maintain a separate “host resolution.”

To force **1:1** logical pixels to screen pixels (e.g. debugging), run with:

`BLUSYS_HOST_ZOOM=1 ./build-host/<your_app>_host`

Values `1`–`8` are accepted; the default is computed from the logical size.

## What's in here

- `CMakeLists.txt` — top-level project. Resolves SDL2 via `pkg-config`, fetches LVGL via `FetchContent`, builds the `blusys_framework_host` bridge library, and registers the `hello_lvgl` + `widget_kit_demo` executables.
- `lv_conf.h` — host LVGL configuration: 32-bit colour, 1 MB memory budget, no RTOS, `LV_USE_LOG=1` with `printf` output, `LV_USE_SDL=1` with the bundled SDL display + input drivers.
- `include_host/esp_log.h` — minimal host shim for ESP-IDF's log header. The framework's `blusys/log.h` wraps `esp_log.h`; on host builds this shim satisfies the include with four `fprintf`-backed macros. Do not grow this shim — keep `blusys/log.h` as the narrow contract instead.
- `src/hello_lvgl.c` — LVGL-only smoke test. Opens an SDL2 window, registers mouse and keyboard input devices, and draws a centred rounded card.
- `src/widget_kit_demo.cpp` — framework bridge demo. Same SDL2 window but the screen is built with `blusys::ui::screen_create`, `col_create`, `button_create`, `slider_create`, `overlay_create` and the full runtime → controller → route sink spine is wired in.

## Also see

- `../host-test/CMakeLists.txt` — minimal headless smoke tests (`ctest` targets with no SDL/LVGL window)

## What's coming next

1. Per-widget visual snapshot tests can build on the headless stub path (`snapshot_buffer_smoke` in `CMakeLists.txt`) and CI.

## Same CLI: firmware, host, and QEMU

Product trees also use **`blusys build . host`** / **`blusys build . qemu32s3`** and **`blusys qemu`** (see **`docs/app/cli-host-qemu.md`**) alongside this monorepo `scripts/host` harness.

## Traceability

**[README.md](../../README.md)** (**Product foundations** → **Validation**) expects host-runnable checks and curated CI. See **`docs/app/validation-host-loop.md`** for a mapping from each goal to this harness, `scripts/scaffold-smoke.sh`, and `.github/workflows/ci.yml`.

## Troubleshooting

- **`Could not find a package configuration file provided by "PkgConfig"`** — install `pkg-config` (`sudo apt install pkg-config`).
- **`No package 'sdl2' found`** — install `libsdl2-dev` (or distro equivalent).
- **CMake fails on `FetchContent`** — check that the host has network access to `github.com` or pre-populate `build-host/_deps/lvgl-src` with a manual checkout of `lvgl/lvgl` at tag `v9.2.2`.
- **`undefined reference to lv_sdl_window_create`** — `LV_USE_SDL` is not set in `lv_conf.h`, or LVGL was configured before `LV_CONF_PATH` was passed. Wipe `build-host/` and re-run `blusys host-build`.
- **`undefined reference to blusys::ui::...` from `bindings.cpp`** — add the matching `blusys::ui` widget `.cpp` to the interactive source list in `cmake/blusys_host_bridge.cmake` (`_BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES`). Run `python3 scripts/check-framework-ui-sources.py` after edits.

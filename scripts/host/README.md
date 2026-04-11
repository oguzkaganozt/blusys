# Blusys host harness — PC + SDL2 + LVGL

Phase 4.5 + Phase 9 of the platform transition. Builds LVGL against SDL2 on
Linux so the Blusys widget kit and framework code can be iterated against
without flashing hardware on every change. The LVGL version is pinned to the
same upstream tag as the ESP-IDF managed component (`lvgl/lvgl ~9.2`, currently
resolving to **v9.2.2** — see `examples/framework_*/dependencies.lock`).

Two executables are built:

- **`hello_lvgl`** (Phase 4.5) — minimal LVGL-only smoke test that proves the
  host toolchain + SDL2 display path works end-to-end.
- **`widget_kit_demo`** (Phase 9) — links the full `blusys_framework` C++
  surface (core spine + V1 widget kit + layout primitives) against the host
  LVGL and builds a screen with `blusys::ui::*` calls rather than raw LVGL.
  Exercises the same runtime → controller → route sink chain as the
  `framework_app_basic` example so Phase 9 stage-1 validation has a concrete
  artifact.

## Prerequisites

The host build needs SDL2 development headers, `cmake >= 3.16`, `pkg-config`,
and a C/C++ toolchain.

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

## Phase 4 validation targets

These ship with the host harness and do not require hardware:

| Binary | Purpose |
|--------|---------|
| `capability_contract_smoke` | Static checks that capability integration event IDs stay in their reserved bands |
| `operational_phase_smoke` | Runtime checks for the edge-node-style operational phase state machine |
| `connected_headless_host` | Headless `blusys::app` loop with connectivity + storage host stubs and a broad `map_event` bridge |

## Build and run

The wrapper goes through the `blusys` CLI:

```bash
blusys host-build                                  # configure + build
./scripts/host/build-host/hello_lvgl               # LVGL-only smoke test
./scripts/host/build-host/widget_kit_demo          # framework widget kit demo
./scripts/host/build-host/operational_phase_smoke  # Phase 4 phase-machine smoke
./scripts/host/build-host/connected_headless_host  # headless capability + reducer smoke
```

`hello_lvgl` opens a 480×320 SDL2 window with a centred rounded card and a
footer hint. Closing the window (or pressing the SDL window close button)
exits via `LV_SDL_DIRECT_EXIT`.

`widget_kit_demo` opens a similar window, but the screen is built entirely
through the framework widget kit: a title + divider + volume slider + three
buttons (`Down` / `Up` / `OK`). Clicking any of the buttons posts a semantic
intent to the runtime; the `Down` / `Up` buttons mutate the slider, and `OK`
submits a `show_overlay` route command that pops a transient toast. Feedback
events are logged to the host console. The mouse acts as the "encoder":
clicks drive the runtime exactly the way a rotary encoder press would on
hardware.

If you'd rather drive CMake directly:

```bash
cd scripts/host
cmake -B build-host -S .
cmake --build build-host -j
./build-host/hello_lvgl
```

The first configure pulls LVGL from `https://github.com/lvgl/lvgl.git` at the
pinned tag via CMake `FetchContent` and caches it under `build-host/_deps/`.
Subsequent builds reuse the cache.

## What's in here

- `CMakeLists.txt` — top-level project. Resolves SDL2 via `pkg-config`, fetches
  LVGL via `FetchContent`, builds the `blusys_framework_host` bridge library,
  and registers the `hello_lvgl` + `widget_kit_demo` executables.
- `lv_conf.h` — host LVGL configuration: 32-bit colour, 1 MB memory budget,
  no RTOS, `LV_USE_LOG=1` with `printf` output, `LV_USE_SDL=1` with the
  bundled SDL display + input drivers.
- `include_host/esp_log.h` — minimal host shim for ESP-IDF's log header. The
  framework's `blusys/log.h` wraps `esp_log.h`; on host builds this shim
  satisfies the include with four `fprintf`-backed macros. Do not grow this
  shim — keep `blusys/log.h` as the narrow contract instead.
- `src/hello_lvgl.c` — Phase 4.5 smoke test. Opens an SDL2 window, registers
  mouse and keyboard input devices, and draws a centred rounded card.
- `src/widget_kit_demo.cpp` — Phase 9 framework bridge demo. Same SDL2 window
  but the screen is built with `blusys::ui::screen_create`,
  `col_create`, `button_create`, `slider_create`, `overlay_create` and
  the full runtime → controller → route sink spine is wired in.

## What's coming next

1. Keyboard-driven encoder simulation in `widget_kit_demo` (map arrow keys to
   `LV_INDEV_TYPE_ENCODER` events) so encoder focus traversal can be
   exercised without hardware.
2. Per-widget visual snapshot tests once a headless display driver is wired
   in alongside the SDL2 one.

## Troubleshooting

- **`Could not find a package configuration file provided by "PkgConfig"`** —
  install `pkg-config` (`sudo apt install pkg-config`).
- **`No package 'sdl2' found`** — install `libsdl2-dev` (or distro equivalent).
- **CMake fails on `FetchContent`** — check that the host has network access
  to `github.com` or pre-populate `build-host/_deps/lvgl-src` with a manual
  checkout of `lvgl/lvgl` at tag `v9.2.2`.
- **`undefined reference to lv_sdl_window_create`** — `LV_USE_SDL` is not set
  in `lv_conf.h`, or LVGL was configured before `LV_CONF_PATH` was passed.
  Wipe `build-host/` and re-run `blusys host-build`.

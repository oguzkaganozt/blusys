# Blusys host harness — PC + SDL2 + LVGL

Phase 4.5 of the platform transition. Builds LVGL against SDL2 on Linux so the
Blusys widget kit and framework code can be iterated against without flashing
hardware on every change. The LVGL version is pinned to the same upstream tag
as the ESP-IDF managed component (`lvgl/lvgl ~9.2`, currently resolving to
**v9.2.2** — see `examples/framework_*/dependencies.lock`).

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

## Build and run

The wrapper goes through the `blusys` CLI:

```bash
blusys host-build           # configure + build into scripts/host/build-host/
./scripts/host/build-host/hello_lvgl
```

A 480×320 SDL2 window opens with a centred rounded card and a footer hint.
Closing the window (or pressing the SDL window close button) exits the program
via `LV_SDL_DIRECT_EXIT`.

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
  LVGL via `FetchContent`, and registers the `hello_lvgl` executable.
- `lv_conf.h` — host LVGL configuration: 32-bit colour, 1 MB memory budget,
  no RTOS, `LV_USE_LOG=1` with `printf` output, `LV_USE_SDL=1` with the
  bundled SDL display + input drivers.
- `src/hello_lvgl.c` — the smoke test. Opens an SDL2 window via
  `lv_sdl_window_create`, registers mouse and keyboard input devices,
  builds a small demo screen, and runs the `lv_tick_inc` / `lv_timer_handler`
  loop until the window is closed.

## What's coming next

The next iterations of this harness will:

1. Link `components/blusys_framework/` into the host build so the widget kit
   can render on the host.
2. Add a `framework_ui_basic` host target that mirrors the embedded example.
3. Add per-widget visual snapshot tests.

These all build on the toolchain proven by the smoke test in this directory.

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

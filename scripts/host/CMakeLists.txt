# scripts/host — PC + SDL2 host harness.
#
# Builds LVGL against SDL2 on Linux so the Blusys widget kit can be
# iterated against without flashing hardware on every change. The
# LVGL version is pinned via FetchContent to the same upstream tag
# as the ESP-IDF managed component (`lvgl/lvgl ~9.2`, currently
# resolving to v9.2.2 — see examples/framework_*/dependencies.lock).
#
# Executables include interactive demos plus host smokes; see the
# target blocks below and scripts/host-test/CMakeLists.txt for the
# minimal no-SDL test loop.
#
# To use:
#   blusys host-build                          # configure + build
#   ./build-host/hello_lvgl                    # LVGL smoke test
#   ./build-host/widget_kit_demo               # framework bridge demo
#
# Requires libsdl2-dev (Debian/Ubuntu) or equivalent on other distros.
# See README.md for the install commands.

cmake_minimum_required(VERSION 3.16)
project(blusys_host_harness LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BLUSYS_HOST_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer for host targets" OFF)
if(BLUSYS_HOST_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -g -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

enable_testing()

# Repo root — used to point the framework bridge at the in-tree
# blusys / blusys_framework component sources.
get_filename_component(BLUSYS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(BLUSYS_PATH "${BLUSYS_REPO_ROOT}")

# All LVGL + SDL2 + blusys_framework_host setup delegates to the shared
# host bridge module so product examples and this monorepo harness stay
# in lockstep. See cmake/blusys_host_bridge.cmake.
include("${BLUSYS_REPO_ROOT}/cmake/blusys_host_bridge.cmake")
blusys_host_bridge_setup_lvgl()
message(STATUS "Found SDL2 ${SDL2_VERSION} via pkg-config")
blusys_host_bridge_add_library(interactive)

# BLUSYS_COMPONENT_DIR (merged component) is set by the bridge include above;
# referenced by the per-target stanzas below.

# ── hello_lvgl smoke test ───────────────────────────────────────────────────
add_executable(hello_lvgl src/hello_lvgl.c)
target_link_libraries(hello_lvgl PRIVATE lvgl PkgConfig::SDL2 m)
blusys_host_bridge_apply_exe_compile_options(hello_lvgl)

# ── widget_kit_demo ───────────────────────────────────────────────────────────
#
# Same LVGL + SDL2 stack as hello_lvgl, but the screen is built with
# `blusys::ui::screen_create` / `col_create` / `button_create` /
# `slider_create` / `overlay_create` instead of raw `lv_obj_create`
# calls. This proves the framework C++ surface compiles and runs
# against the host LVGL and gives us a fast visual iteration loop
# for the widget kit.
add_executable(widget_kit_demo src/widget_kit_demo.cpp)
target_link_libraries(widget_kit_demo PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
blusys_host_bridge_apply_exe_compile_options(widget_kit_demo)

# ── app_interactive_demo (blusys::app interactive consumer) ───────────────────
add_executable(app_interactive_demo
    src/app_interactive_demo.cpp
    src/app_host_platform.cpp
)
target_link_libraries(app_interactive_demo PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
blusys_host_bridge_apply_exe_compile_options(app_interactive_demo)

# ── app_headless_demo (blusys::app headless sensor node) ────────────────────
add_executable(app_headless_demo
    src/app_headless_demo.cpp
    src/app_headless_platform.cpp
)
target_link_libraries(app_headless_demo PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
blusys_host_bridge_apply_exe_compile_options(app_headless_demo)
# Prevent SDL from hijacking main() — the headless path owns its own entry.
target_compile_definitions(app_headless_demo PRIVATE SDL_MAIN_HANDLED)

# ── theme_presets_smoke (preset + visual feedback token sanity) ──────────────
add_executable(theme_presets_smoke src/theme_presets_smoke.cpp)
target_link_libraries(theme_presets_smoke PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
blusys_host_bridge_apply_exe_compile_options(theme_presets_smoke)

# ── connected_headless_host (connected product on host) ─────────────────────
#
# Exercises the connectivity and storage capability host stubs through the
# same reducer event flow used on device. Proves the full capability
# lifecycle runs on host without hardware.
add_executable(connected_headless_host
    src/connected_headless_host.cpp
    src/app_headless_platform.cpp
)
target_link_libraries(connected_headless_host PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
blusys_host_bridge_apply_exe_compile_options(connected_headless_host)
target_compile_definitions(connected_headless_host PRIVATE SDL_MAIN_HANDLED)

# ── capability_contract_smoke (enum range sanity) ─────────────────────────────
add_executable(capability_contract_smoke src/capability_contract_smoke.cpp)
target_include_directories(capability_contract_smoke PRIVATE
    ${BLUSYS_COMPONENT_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include_host
)
blusys_host_bridge_apply_exe_compile_options(capability_contract_smoke)

# ── operational_phase_smoke (headless-telemetry operational phase machine parity) ─
add_executable(operational_phase_smoke src/operational_phase_smoke.cpp)
blusys_host_apply_compile_options(operational_phase_smoke)

# ── handheld_starter_reducer_smoke (shared clamp_percent) ─────────────────────
add_executable(handheld_starter_reducer_smoke src/handheld_starter_reducer_smoke.cpp)
target_include_directories(handheld_starter_reducer_smoke PRIVATE
    ${BLUSYS_REPO_ROOT}/examples/include
)
blusys_host_apply_compile_options(handheld_starter_reducer_smoke)

# ── panel_connectivity_map_smoke (panel map_event connectivity band) ──────────
add_executable(panel_connectivity_map_smoke src/panel_connectivity_map_smoke.cpp)
target_include_directories(panel_connectivity_map_smoke PRIVATE
    ${BLUSYS_REPO_ROOT}/examples/include
    ${BLUSYS_COMPONENT_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include_host
)
blusys_host_bridge_apply_exe_compile_options(panel_connectivity_map_smoke)

# ── pomodoro_host (oled_encoder_basic ported to host) ────────────────────────
#
# Pixel-identical 128×32 OLED layout running on the PC + SDL2 stack.
# The window opens at 4× zoom (512×128 px) for desktop legibility.
# Keyboard input replaces the physical rotary encoder:
#   ← / → arrows  → CCW / CW  (set duration)
#   Space / Enter  → PRESS     (start / stop)
add_executable(pomodoro_host src/pomodoro_host_main.cpp)
target_link_libraries(pomodoro_host PRIVATE lvgl PkgConfig::SDL2 m)
blusys_host_bridge_apply_exe_compile_options(pomodoro_host)

# ── host_spine_test (ring_buffer / containers — no LVGL) ─────────────────────
add_executable(host_spine_test src/host_spine_test.cpp)
target_include_directories(host_spine_test PRIVATE ${BLUSYS_COMPONENT_DIR}/include)
blusys_host_apply_warning_options(host_spine_test)
add_test(NAME host_spine_test COMMAND host_spine_test)

# ── action_queue_drop_smoke (app_runtime action queue overflow counter) ───────
add_executable(action_queue_drop_smoke
    src/action_queue_drop_smoke.cpp
    src/app_headless_platform.cpp
)
target_link_libraries(action_queue_drop_smoke PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
blusys_host_bridge_apply_exe_compile_options(action_queue_drop_smoke)
target_compile_definitions(action_queue_drop_smoke PRIVATE SDL_MAIN_HANDLED)
add_test(NAME action_queue_drop_smoke COMMAND action_queue_drop_smoke)

# ── snapshot_buffer_smoke (stub display + partial flush, no SDL window) ───────
add_executable(snapshot_buffer_smoke src/snapshot_buffer_smoke.cpp)
target_link_libraries(snapshot_buffer_smoke PRIVATE lvgl m)
blusys_host_bridge_apply_exe_compile_options(snapshot_buffer_smoke)
add_test(NAME snapshot_buffer_smoke COMMAND snapshot_buffer_smoke)

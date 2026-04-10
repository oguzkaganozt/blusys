# scripts/host — PC + SDL2 host harness for Phase 4.5 + Phase 9.
#
# Builds LVGL against SDL2 on Linux so the Blusys widget kit can be
# iterated against without flashing hardware on every change. The
# LVGL version is pinned via FetchContent to the same upstream tag
# as the ESP-IDF managed component (`lvgl/lvgl ~9.2`, currently
# resolving to v9.2.2 — see examples/framework_*/dependencies.lock).
#
# Two executables:
#   hello_lvgl        — Phase 4.5 LVGL-only smoke test.
#   widget_kit_demo   — Phase 9 framework bridge: compiles the full
#                       blusys_framework C++ surface (core spine +
#                       widget kit) against the host LVGL + SDL2 and
#                       exercises the V1 widgets through the same
#                       spine path a device build uses.
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

# Repo root — used to point the framework bridge at the in-tree
# blusys / blusys_framework component sources.
get_filename_component(BLUSYS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# ── SDL2 (via pkg-config — most portable on Linux) ───────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

message(STATUS "Found SDL2 ${SDL2_VERSION} via pkg-config")

# ── LVGL (FetchContent, pinned to the same tag as the managed component) ─────
include(FetchContent)

# Tell LVGL where our lv_conf.h lives. LVGL's `custom.cmake` honours
# `LV_CONF_PATH` and adds the parent directory to its include path
# automatically.
set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE FILEPATH "Path to lv_conf.h")

# We don't need thorvg, the LVGL examples library, or the demos library
# for the host harness — disabling them avoids pulling in extra
# dependencies and shortens the build.
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES        ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS           ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(lvgl)

# LVGL's `custom.cmake` collects sources via `file(GLOB_RECURSE
# src/*.c src/*.S)`, which unconditionally pulls in the ARM NEON
# (`lv_blend_neon.S`) and Cortex-M Helium (`lv_blend_helium.S`)
# assembly back-ends regardless of host architecture. On x86_64
# the assembler chokes on the C-style typedefs that arrive via the
# preprocessor expansion of `<stdint.h>`. The C alternatives are
# always available — the assembly back-ends are guarded by
# `LV_USE_DRAW_SW_ASM`, which defaults to `LV_DRAW_SW_ASM_NONE` —
# so we can safely strip every `.S` file from the lvgl target on
# any non-ARM host.
get_target_property(_lvgl_sources lvgl SOURCES)
list(FILTER _lvgl_sources EXCLUDE REGEX "\\.S$")
set_property(TARGET lvgl PROPERTY SOURCES ${_lvgl_sources})

# LVGL's SDL driver source files (src/drivers/sdl/*.c) are pulled in
# by LVGL's `file(GLOB_RECURSE)` automatically when LV_USE_SDL is set
# in our lv_conf.h. They #include <SDL2/SDL.h>, so the lvgl target
# itself needs the SDL2 include path and library — link it on PUBLIC
# so downstream targets like hello_lvgl get them transitively.
target_link_libraries(lvgl PUBLIC PkgConfig::SDL2)

# ── blusys_framework bridge (Phase 9) ────────────────────────────────────────
#
# Compiles the framework C++ sources + the small C dependency they
# need from the HAL component (`blusys_err_string`) into a single
# static library that host executables can link against. The
# framework's only non-LVGL external dependency is `blusys/log.h`,
# which transitively includes `esp_log.h`; we satisfy that include
# on host via the `include_host/esp_log.h` shim.
#
# Compile options mirror `components/blusys_framework/CMakeLists.txt`
# so host and device builds share the same warning surface, C++
# dialect (C++20), and the no-exceptions / no-RTTI policy the
# widget kit relies on. If you change one, change the other.

set(BLUSYS_COMPONENT_DIR      "${BLUSYS_REPO_ROOT}/components/blusys")
set(BLUSYS_FRAMEWORK_DIR      "${BLUSYS_REPO_ROOT}/components/blusys_framework")

set(blusys_framework_host_sources
    # Minimal HAL dependency: the framework's public headers use
    # `blusys_err_t` and `blusys_err_string`. The enum lives in a
    # C header that has zero ESP-IDF deps, but `blusys_err_string`
    # has its definition in src/common/error.c — compile it in so
    # the host library is link-complete on its own.
    ${BLUSYS_COMPONENT_DIR}/src/common/error.c

    # Core spine — C++, pure logic, no LVGL deps.
    ${BLUSYS_FRAMEWORK_DIR}/src/core/controller.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/feedback.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/feedback_presets.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/framework.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/router.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/runtime.cpp

    # App layer — blusys::app product-facing API.
    ${BLUSYS_FRAMEWORK_DIR}/src/app/app_ctx.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/connectivity_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/storage_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/theme_presets.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/action_widgets.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/bindings.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/overlay_manager.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/page.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/screen_registry.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/screen_router.cpp

    # UI surface — LVGL-backed. Always on in the host harness
    # (device builds gate this behind BLUSYS_BUILD_UI).
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/icons/icon_set_minimal.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/input/encoder.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/primitives/col.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/primitives/divider.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/primitives/label.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/primitives/row.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/primitives/screen.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/theme.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/widgets/button/button.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/widgets/modal/modal.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/widgets/overlay/overlay.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/widgets/slider/slider.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/ui/widgets/toggle/toggle.cpp
)

add_library(blusys_framework_host STATIC ${blusys_framework_host_sources})

target_include_directories(blusys_framework_host PUBLIC
    ${BLUSYS_COMPONENT_DIR}/include
    ${BLUSYS_FRAMEWORK_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include_host
)

# Match the device-side compile policy (see blusys_framework/CMakeLists.txt).
# -fno-rtti and -fno-threadsafe-statics are C++-only, so gate them
# behind the CXX compile-language generator expression — blusys_err_string
# (the one C source pulled into this target) would otherwise produce a
# `cc1: warning: command-line option '-fno-rtti' is valid for C++/D/ObjC++
# but not for C` noise line on every configure.
target_compile_options(blusys_framework_host PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    -Wall
    -Wextra
)

target_link_libraries(blusys_framework_host PUBLIC lvgl)
target_compile_definitions(blusys_framework_host PUBLIC BLUSYS_FRAMEWORK_HAS_UI=1)

# ── hello_lvgl smoke test (Phase 4.5) ────────────────────────────────────────
add_executable(hello_lvgl src/hello_lvgl.c)
target_link_libraries(hello_lvgl PRIVATE lvgl PkgConfig::SDL2 m)
target_compile_options(hello_lvgl PRIVATE -Wall -Wextra)

# ── widget_kit_demo (Phase 9) ────────────────────────────────────────────────
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
target_compile_options(widget_kit_demo PRIVATE
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Wall
    -Wextra
)

# ── app_interactive_demo (Phase 1 — blusys::app interactive consumer) ────────
add_executable(app_interactive_demo
    src/app_interactive_demo.cpp
    src/app_host_platform.cpp
)
target_link_libraries(app_interactive_demo PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
target_compile_options(app_interactive_demo PRIVATE
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Wall
    -Wextra
)

# ── app_headless_demo (Phase 1 — blusys::app headless sensor node) ───────────
add_executable(app_headless_demo
    src/app_headless_demo.cpp
    src/app_headless_platform.cpp
)
target_link_libraries(app_headless_demo PRIVATE
    blusys_framework_host
    PkgConfig::SDL2
    m
)
target_compile_options(app_headless_demo PRIVATE
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Wall
    -Wextra
)
# Prevent SDL from hijacking main() — the headless path owns its own entry.
target_compile_definitions(app_headless_demo PRIVATE SDL_MAIN_HANDLED)

# ── connected_headless_host (Phase 4 — connected product on host) ───────────
#
# Exercises the connectivity and storage bundle host stubs through the
# same reducer event flow used on device. Proves the full bundle
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
target_compile_options(connected_headless_host PRIVATE
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Wall
    -Wextra
)
target_compile_definitions(connected_headless_host PRIVATE SDL_MAIN_HANDLED)

# ── pomodoro_host (oled_encoder_basic ported to host) ────────────────────────
#
# Pixel-identical 128×32 OLED layout running on the PC + SDL2 stack.
# The window opens at 4× zoom (512×128 px) for desktop legibility.
# Keyboard input replaces the physical rotary encoder:
#   ← / → arrows  → CCW / CW  (set duration)
#   Space / Enter  → PRESS     (start / stop)
add_executable(pomodoro_host src/pomodoro_host_main.cpp)
target_link_libraries(pomodoro_host PRIVATE lvgl PkgConfig::SDL2 m)
target_compile_options(pomodoro_host PRIVATE
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Wall
    -Wextra
)

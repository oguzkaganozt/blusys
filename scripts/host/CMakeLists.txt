# scripts/host — PC + SDL2 host harness for Phase 4.5.
#
# Builds LVGL against SDL2 on Linux so the Blusys widget kit can be
# iterated against without flashing hardware on every change. The
# LVGL version is pinned via FetchContent to the same upstream tag
# as the ESP-IDF managed component (`lvgl/lvgl ~9.2`, currently
# resolving to v9.2.2 — see examples/framework_*/dependencies.lock).
#
# To use:
#   blusys host-build              # configure + build
#   ./build-host/hello_lvgl        # run the smoke test
#
# Requires libsdl2-dev (Debian/Ubuntu) or equivalent on other distros.
# See README.md for the install commands.

cmake_minimum_required(VERSION 3.16)
project(blusys_host_harness LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# ── hello_lvgl smoke test ────────────────────────────────────────────────────
add_executable(hello_lvgl src/hello_lvgl.c)
target_link_libraries(hello_lvgl PRIVATE lvgl PkgConfig::SDL2 m)
target_compile_options(hello_lvgl PRIVATE -Wall -Wextra)

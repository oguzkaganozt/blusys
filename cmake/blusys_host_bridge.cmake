# blusys host bridge — single source of truth for LVGL/SDL2 + framework static
# library setup used by `scripts/host/` and every example's `host/CMakeLists.txt`.
#
# Two entry points:
#
#   blusys_host_bridge_setup_lvgl()
#     Declares SDL2 (pkg-config) and LVGL (FetchContent v9.2.2 with .S filter).
#     Must be called before `blusys_host_bridge_add_library()` for either mode.
#
#   blusys_host_bridge_add_library(MODE [<library_name>])
#     MODE is either `interactive` (LVGL + UI sources) or `headless`
#     (spine + capabilities only; no LVGL deps). Default library name is
#     `blusys_framework_host` (interactive) or `blusys_framework_core_host`
#     (headless). Both interactive and headless build the full capability
#     host set — only the UI surface differs. This matches the superset
#     already compiled by scripts/host/CMakeLists.txt and avoids every
#     example having to curate its own capability list.
#
# Repo root: set `BLUSYS_PATH` before include, **or** export `BLUSYS_PATH` in the
# environment and use `include("$ENV{BLUSYS_PATH}/cmake/blusys_host_bridge.cmake")`.
#
# Compile policy mirrors components/blusys_framework/CMakeLists.txt:
# -fno-exceptions, -fno-rtti, -fno-threadsafe-statics (C++ only),
# -Wall, -Wextra.

if(NOT BLUSYS_PATH)
    if(DEFINED ENV{BLUSYS_PATH} AND NOT "$ENV{BLUSYS_PATH}" STREQUAL "")
        set(BLUSYS_PATH "$ENV{BLUSYS_PATH}")
    endif()
endif()
if(NOT BLUSYS_PATH)
    message(FATAL_ERROR
        "blusys_host_bridge.cmake: set BLUSYS_PATH CMake variable, or export BLUSYS_PATH "
        "(e.g. run via 'blusys host-build').")
endif()

set(BLUSYS_COMPONENT_DIR  "${BLUSYS_PATH}/components/blusys")
set(BLUSYS_FRAMEWORK_DIR  "${BLUSYS_PATH}/components/blusys_framework")
set(BLUSYS_SERVICES_DIR   "${BLUSYS_PATH}/components/blusys_services")

# ── LVGL + SDL2 setup (interactive mode only) ───────────────────────────────
function(blusys_host_bridge_setup_lvgl)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

    include(FetchContent)
    set(LV_CONF_PATH "${BLUSYS_PATH}/scripts/host/lv_conf.h"
        CACHE FILEPATH "Path to lv_conf.h")
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

    # LVGL's custom.cmake unconditionally globs ARM NEON/Helium assembly
    # (lv_blend_neon.S / lv_blend_helium.S). Strip .S on non-ARM hosts —
    # the C alternatives are always available because LV_USE_DRAW_SW_ASM
    # defaults to LV_DRAW_SW_ASM_NONE.
    get_target_property(_lvgl_sources lvgl SOURCES)
    list(FILTER _lvgl_sources EXCLUDE REGEX "\\.S$")
    set_property(TARGET lvgl PROPERTY SOURCES ${_lvgl_sources})

    target_link_libraries(lvgl PUBLIC PkgConfig::SDL2)
endfunction()

# ── Common framework sources (mode-independent) ─────────────────────────────
set(_BLUSYS_HOST_BRIDGE_COMMON_SOURCES
    ${BLUSYS_COMPONENT_DIR}/src/common/error.c
    ${BLUSYS_FRAMEWORK_DIR}/src/core/feedback.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/feedback_presets.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/feedback_logging_sink.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/framework.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/router.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/core/runtime.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/app_ctx.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/app_services.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capability_event_map.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/connectivity_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/storage_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/bluetooth_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/ota_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/diagnostics_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/provisioning_host.cpp
    ${BLUSYS_FRAMEWORK_DIR}/src/app/capabilities/telemetry.cpp
)

function(blusys_host_bridge_add_library MODE)
    if(ARGC GREATER 2)
        message(FATAL_ERROR "blusys_host_bridge_add_library: too many arguments (got ${ARGC}, expected 1-2). Usage: blusys_host_bridge_add_library(<interactive|headless> [<target-name>])")
    endif()
    if(NOT MODE STREQUAL "interactive" AND NOT MODE STREQUAL "headless")
        message(FATAL_ERROR
            "blusys_host_bridge_add_library: MODE must be 'interactive' or 'headless' (got '${MODE}')")
    endif()

    set(lib_name "${ARGV1}")
    if(NOT lib_name)
        if(MODE STREQUAL "interactive")
            set(lib_name "blusys_framework_host")
        else()
            set(lib_name "blusys_framework_core_host")
        endif()
    endif()

    set(_sources ${_BLUSYS_HOST_BRIDGE_COMMON_SOURCES})

    if(MODE STREQUAL "interactive")
        include("${BLUSYS_PATH}/cmake/blusys_framework_ui_sources.cmake")
        foreach(_rel IN LISTS BLUSYS_FRAMEWORK_UI_SRC_SHARED)
            list(APPEND _sources "${BLUSYS_FRAMEWORK_DIR}/${_rel}")
        endforeach()
    endif()

    add_library(${lib_name} STATIC ${_sources})

    target_include_directories(${lib_name} PUBLIC
        ${BLUSYS_COMPONENT_DIR}/include
        ${BLUSYS_FRAMEWORK_DIR}/include
        ${BLUSYS_SERVICES_DIR}/include
        ${BLUSYS_PATH}/scripts/host/include_host
    )

    target_compile_options(${lib_name} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
        -Wall
        -Wextra
    )

    if(MODE STREQUAL "interactive")
        target_link_libraries(${lib_name} PUBLIC lvgl)
        target_compile_definitions(${lib_name} PUBLIC BLUSYS_FRAMEWORK_HAS_UI=1)
    else()
        # theme_presets.cpp is deliberately excluded from headless builds: it
        # depends on LVGL colour types (lv_color_t) and the UI theme layer which
        # are not compiled in headless mode.  Headless consumers that need colour
        # token constants should pull them directly or use a separate stub.
        #
        # Headless host owns its own entry (no SDL window).
        target_link_libraries(${lib_name} PUBLIC PkgConfig::SDL2)
        target_compile_definitions(${lib_name} PUBLIC SDL_MAIN_HANDLED)
    endif()
endfunction()

# ── Compile options shared by product host executables ──────────────────────
#
# Usage:
#   blusys_host_bridge_apply_exe_compile_options(<target>)
function(blusys_host_bridge_apply_exe_compile_options target)
    target_compile_options(${target} PRIVATE
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
        -Wall
        -Wextra
    )
endfunction()

# ── Build version tag (for BLUSYS_APP_BUILD_VERSION defines) ────────────────
function(blusys_host_bridge_resolve_build_version OUT_VAR)
    execute_process(
        COMMAND git -C "${BLUSYS_PATH}" describe --tags --dirty --always
        OUTPUT_VARIABLE _ver
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT _ver)
        set(_ver "host-build")
    endif()
    set(${OUT_VAR} "${_ver}" PARENT_SCOPE)
endfunction()

# ── BLUSYS_PATH check for product host builds (ENV-driven) ──────────────────
#
# Optional: call when you need an explicit fatal error before any include.
# Otherwise `include("$ENV{BLUSYS_PATH}/cmake/blusys_host_bridge.cmake")` and
# the bridge init above are enough if `BLUSYS_PATH` is exported.
macro(blusys_require_blusys_path_env)
    if(NOT DEFINED ENV{BLUSYS_PATH} OR "$ENV{BLUSYS_PATH}" STREQUAL "")
        message(FATAL_ERROR
            "BLUSYS_PATH is not set.\n"
            "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
            "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
    endif()
endmacro()

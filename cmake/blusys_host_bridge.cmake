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
# Compile policy mirrors components/blusys/CMakeLists.txt:
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

# Single merged component directory (v0 architecture).
set(BLUSYS_COMPONENT_DIR "${BLUSYS_PATH}/components/blusys")

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
    ${BLUSYS_COMPONENT_DIR}/src/hal/error.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/observe/error_domain.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/observe/log.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/observe/counter.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/feedback/feedback.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/feedback/presets.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/feedback/internal/logging_sink.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/engine/framework_init.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/engine/router.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/engine/event_queue.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/app/ctx.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/app/services.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/capability_event_map.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/ble_hid_device_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/connectivity_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/storage_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/bluetooth_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/ota_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/diagnostics_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/lan_control_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/provisioning_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/usb_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/mqtt_host.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/telemetry.cpp
)

# ── Interactive-mode sources (LVGL UI surface) ───────────────────────────────
# Flows are included here — they depend on LVGL types only in device builds;
# the host stubs in scripts/host/include_host provide the missing shims.
set(_BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES
    # flows
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/boot.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/loading.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/error.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/status.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/settings.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/ota_flow.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/provisioning_flow.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/connectivity_flow.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/flows/diagnostics_flow.cpp
    # platform bridges (touch + input compile on host; button_array + device platform do not)
    ${BLUSYS_COMPONENT_DIR}/src/framework/platform/touch_bridge.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/platform/input_bridge.cpp
    # UI — style
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/style/theme.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/style/presets.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/style/transition.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/style/interaction_effects.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/style/icon_set_minimal.cpp
    # UI — input
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/input/encoder.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/input/focus_scope.cpp
    # UI — composition
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/composition/overlay_manager.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/composition/page.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/composition/screen_registry.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/composition/screen_router.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/composition/shell.cpp
    # UI — binding
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/binding/bindings.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/binding/action_widgets.cpp
    # UI — primitives
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/col.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/divider.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/icon_label.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/key_value.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/label.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/row.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/screen.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/primitives/status_badge.cpp
    # UI — widgets (flat)
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/button.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/card.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/chart.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/data_table.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/dropdown.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/gauge.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/input_field.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/knob.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/level_bar.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/list.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/modal.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/overlay.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/progress.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/slider.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/tabs.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/toggle.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/widgets/vu_strip.cpp
    # UI — screens
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/screens/about.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/screens/diagnostics.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/ui/screens/status.cpp
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

    # Common host sources now include MQTT capability glue, so every host
    # framework library needs libmosquitto on its link line.
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MOSQUITTO QUIET IMPORTED_TARGET libmosquitto)
    if(NOT MOSQUITTO_FOUND)
        set(_blusys_pkgconfig_path "$ENV{PKG_CONFIG_PATH}")
        if(_blusys_pkgconfig_path STREQUAL "")
            set(_blusys_pkgconfig_path "/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig")
        else()
            set(_blusys_pkgconfig_path "/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig:${_blusys_pkgconfig_path}")
        endif()
        set(ENV{PKG_CONFIG_PATH} "${_blusys_pkgconfig_path}")
        pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)
    endif()

    if(MODE STREQUAL "interactive")
        list(APPEND _sources ${_BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES})
    endif()

    add_library(${lib_name} STATIC ${_sources})

    target_include_directories(${lib_name} PUBLIC
        ${BLUSYS_COMPONENT_DIR}/include
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

    target_link_libraries(${lib_name} PUBLIC PkgConfig::MOSQUITTO)
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

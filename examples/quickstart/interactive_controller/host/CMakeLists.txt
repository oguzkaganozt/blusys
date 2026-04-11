cmake_minimum_required(VERSION 3.16)
project(interactive_controller_host LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{BLUSYS_PATH})
    message(FATAL_ERROR
        "BLUSYS_PATH is not set.\n"
        "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
        "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
endif()
set(BLUSYS_PATH "$ENV{BLUSYS_PATH}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

include(FetchContent)
set(LV_CONF_PATH "${BLUSYS_PATH}/scripts/host/lv_conf.h" CACHE FILEPATH "")
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
FetchContent_Declare(lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(lvgl)
get_target_property(_lvgl_src lvgl SOURCES)
list(FILTER _lvgl_src EXCLUDE REGEX "\\.S$")
set_property(TARGET lvgl PROPERTY SOURCES ${_lvgl_src})
target_link_libraries(lvgl PUBLIC PkgConfig::SDL2)

set(BLUSYS_COMP "${BLUSYS_PATH}/components/blusys")
set(BLUSYS_FW   "${BLUSYS_PATH}/components/blusys_framework")
set(BLUSYS_SVC  "${BLUSYS_PATH}/components/blusys_services")

add_library(blusys_framework_host STATIC
    ${BLUSYS_COMP}/src/common/error.c
    ${BLUSYS_FW}/src/core/controller.cpp
    ${BLUSYS_FW}/src/core/feedback.cpp
    ${BLUSYS_FW}/src/core/feedback_presets.cpp
    ${BLUSYS_FW}/src/core/framework.cpp
    ${BLUSYS_FW}/src/core/router.cpp
    ${BLUSYS_FW}/src/core/runtime.cpp
    ${BLUSYS_FW}/src/app/app_ctx.cpp
    ${BLUSYS_FW}/src/app/capabilities/connectivity_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/storage_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/provisioning_host.cpp
    ${BLUSYS_FW}/src/app/theme_presets.cpp
    ${BLUSYS_FW}/src/app/action_widgets.cpp
    ${BLUSYS_FW}/src/app/bindings.cpp
    ${BLUSYS_FW}/src/app/overlay_manager.cpp
    ${BLUSYS_FW}/src/app/page.cpp
    ${BLUSYS_FW}/src/app/screen_registry.cpp
    ${BLUSYS_FW}/src/app/screen_router.cpp
    ${BLUSYS_FW}/src/app/shell.cpp
    ${BLUSYS_FW}/src/app/flows/settings.cpp
    ${BLUSYS_FW}/src/app/flows/provisioning_flow.cpp
    ${BLUSYS_FW}/src/app/screens/about_screen.cpp
    ${BLUSYS_FW}/src/ui/icons/icon_set_minimal.cpp
    ${BLUSYS_FW}/src/ui/input/encoder.cpp
    ${BLUSYS_FW}/src/ui/input/focus_scope.cpp
    ${BLUSYS_FW}/src/ui/primitives/col.cpp
    ${BLUSYS_FW}/src/ui/primitives/divider.cpp
    ${BLUSYS_FW}/src/ui/primitives/icon_label.cpp
    ${BLUSYS_FW}/src/ui/primitives/key_value.cpp
    ${BLUSYS_FW}/src/ui/primitives/label.cpp
    ${BLUSYS_FW}/src/ui/primitives/row.cpp
    ${BLUSYS_FW}/src/ui/primitives/screen.cpp
    ${BLUSYS_FW}/src/ui/primitives/status_badge.cpp
    ${BLUSYS_FW}/src/ui/theme.cpp
    ${BLUSYS_FW}/src/ui/transition.cpp
    ${BLUSYS_FW}/src/ui/widgets/button/button.cpp
    ${BLUSYS_FW}/src/ui/widgets/card/card.cpp
    ${BLUSYS_FW}/src/ui/widgets/data_table/data_table.cpp
    ${BLUSYS_FW}/src/ui/widgets/dropdown/dropdown.cpp
    ${BLUSYS_FW}/src/ui/widgets/gauge/gauge.cpp
    ${BLUSYS_FW}/src/ui/widgets/input_field/input_field.cpp
    ${BLUSYS_FW}/src/ui/widgets/list/list.cpp
    ${BLUSYS_FW}/src/ui/widgets/modal/modal.cpp
    ${BLUSYS_FW}/src/ui/widgets/overlay/overlay.cpp
    ${BLUSYS_FW}/src/ui/widgets/progress/progress.cpp
    ${BLUSYS_FW}/src/ui/widgets/slider/slider.cpp
    ${BLUSYS_FW}/src/ui/widgets/tabs/tabs.cpp
    ${BLUSYS_FW}/src/ui/widgets/toggle/toggle.cpp
)
target_include_directories(blusys_framework_host PUBLIC
    ${BLUSYS_COMP}/include
    ${BLUSYS_FW}/include
    ${BLUSYS_SVC}/include
    ${BLUSYS_PATH}/scripts/host/include_host
)
target_compile_options(blusys_framework_host PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    -Wall -Wextra
)
target_link_libraries(blusys_framework_host PUBLIC lvgl)
target_compile_definitions(blusys_framework_host PUBLIC BLUSYS_FRAMEWORK_HAS_UI=1)

add_executable(interactive_controller_host
    "${CMAKE_CURRENT_LIST_DIR}/../main/logic/app_logic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/ui/app_ui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/system/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_host_platform.cpp"
)
target_include_directories(interactive_controller_host PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
)
target_link_libraries(interactive_controller_host PRIVATE
    blusys_framework_host PkgConfig::SDL2 m
)
target_compile_options(interactive_controller_host PRIVATE
    -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -Wextra
)

# Mirror device profile for SDL window size: 0 = ST7735-class (240×240), 1 = ST7789 (320×240).
set(BLUSYS_IC_HOST_DISPLAY_PROFILE "0" CACHE STRING "Host SDL: 0=240x240 1=320x240")
target_compile_definitions(interactive_controller_host PRIVATE
    BLUSYS_IC_HOST_DISPLAY_PROFILE=${BLUSYS_IC_HOST_DISPLAY_PROFILE})

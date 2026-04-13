cmake_minimum_required(VERSION 3.16)
project(handheld_bluetooth_host LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{BLUSYS_PATH} OR "$ENV{BLUSYS_PATH}" STREQUAL "")
    message(FATAL_ERROR
        "BLUSYS_PATH is not set.\n"
        "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
        "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
endif()

include("$ENV{BLUSYS_PATH}/cmake/blusys_host_bridge.cmake")
blusys_host_bridge_setup_lvgl()
blusys_host_bridge_add_library(interactive)
blusys_host_bridge_resolve_build_version(BLUSYS_GIT_VERSION)

add_executable(handheld_bluetooth_host
    "${CMAKE_CURRENT_LIST_DIR}/../main/core/app_logic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/ui/app_ui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/integration/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_host_platform.cpp"
)
target_include_directories(handheld_bluetooth_host PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
    "${BLUSYS_PATH}/examples/include"
)
target_link_libraries(handheld_bluetooth_host PRIVATE
    blusys_framework_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options(handheld_bluetooth_host)
target_compile_definitions(handheld_bluetooth_host PRIVATE
    BLUSYS_APP_BUILD_VERSION="${BLUSYS_GIT_VERSION}")

set(BLUSYS_IC_HOST_DISPLAY_PROFILE "0" CACHE STRING "Host SDL: 0=240x240 1=320x240")
target_compile_definitions(handheld_bluetooth_host PRIVATE
    BLUSYS_IC_HOST_DISPLAY_PROFILE=${BLUSYS_IC_HOST_DISPLAY_PROFILE})

cmake_minimum_required(VERSION 3.16)

project(external_lockdown_host LANGUAGES C CXX)

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

set(BLUSYS_PATH "$ENV{BLUSYS_PATH}")

include("$ENV{BLUSYS_PATH}/cmake/blusys_host_bridge.cmake")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

blusys_host_bridge_add_library(headless)

add_executable(external_lockdown_host
    "${CMAKE_CURRENT_LIST_DIR}/../main/platform/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_headless_platform.cpp"
)
target_include_directories(external_lockdown_host PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
)
target_link_libraries(external_lockdown_host PRIVATE
    blusys_framework_core_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options(external_lockdown_host)

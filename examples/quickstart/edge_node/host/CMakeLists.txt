cmake_minimum_required(VERSION 3.16)
project(edge_node_host LANGUAGES C CXX)

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

set(BLUSYS_COMP "${BLUSYS_PATH}/components/blusys")
set(BLUSYS_FW   "${BLUSYS_PATH}/components/blusys_framework")

add_library(blusys_framework_core_host STATIC
    ${BLUSYS_COMP}/src/common/error.c
    ${BLUSYS_FW}/src/core/controller.cpp
    ${BLUSYS_FW}/src/core/feedback.cpp
    ${BLUSYS_FW}/src/core/feedback_presets.cpp
    ${BLUSYS_FW}/src/app/feedback_logging_sink.cpp
    ${BLUSYS_FW}/src/core/framework.cpp
    ${BLUSYS_FW}/src/core/router.cpp
    ${BLUSYS_FW}/src/core/runtime.cpp
    ${BLUSYS_FW}/src/app/app_ctx.cpp
    ${BLUSYS_FW}/src/app/bind_capability_ptrs.cpp
    ${BLUSYS_FW}/src/app/capabilities/connectivity_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/storage_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/provisioning_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/diagnostics_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/ota_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/bluetooth_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/telemetry.cpp
)
target_include_directories(blusys_framework_core_host PUBLIC
    ${BLUSYS_COMP}/include
    ${BLUSYS_FW}/include
    ${BLUSYS_PATH}/scripts/host/include_host
)
target_compile_options(blusys_framework_core_host PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    -Wall -Wextra
)
target_compile_definitions(blusys_framework_core_host PUBLIC SDL_MAIN_HANDLED)

add_executable(edge_node_host
    "${CMAKE_CURRENT_LIST_DIR}/../main/core/app_logic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/integration/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_headless_platform.cpp"
)
target_include_directories(edge_node_host PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
)
target_link_libraries(edge_node_host PRIVATE
    blusys_framework_core_host PkgConfig::SDL2 m
)
target_compile_options(edge_node_host PRIVATE
    -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -Wextra
)

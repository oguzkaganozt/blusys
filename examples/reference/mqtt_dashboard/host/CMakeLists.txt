cmake_minimum_required(VERSION 3.16)
project(mqtt_dashboard_host LANGUAGES C CXX)

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

find_package(PkgConfig REQUIRED)
pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)
include("${BLUSYS_PATH}/cmake/blusys_framework_host_mqtt.cmake")
target_sources(blusys_framework_host PRIVATE ${BLUSYS_FRAMEWORK_HOST_MQTT_SOURCES})
target_link_libraries(blusys_framework_host PUBLIC PkgConfig::MOSQUITTO)

set(BLUSYS_MQTT_DASH_BROKER "mqtt://test.mosquitto.org" CACHE STRING "MQTT broker URI for host demo")

add_executable(mqtt_dashboard_host
    "${CMAKE_CURRENT_LIST_DIR}/../main/core/app_logic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/ui/app_ui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/platform/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_host_platform.cpp"
)
target_include_directories(mqtt_dashboard_host PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
    "${BLUSYS_PATH}/examples/include"
)
target_link_libraries(mqtt_dashboard_host PRIVATE
    blusys_framework_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options(mqtt_dashboard_host)
target_compile_definitions(mqtt_dashboard_host PRIVATE
    BLUSYS_APP_BUILD_VERSION="${BLUSYS_GIT_VERSION}"
    "BLUSYS_MQTT_DASH_BROKER=\"${BLUSYS_MQTT_DASH_BROKER}\""
)

# 0 = 320x240, 1 = 480x320 (matches device Kconfig naming used in app_main).
set(BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE "1" CACHE STRING "Host SDL: 0=320x240 1=480x320")
target_compile_definitions(mqtt_dashboard_host PRIVATE
    BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE=${BLUSYS_DASHBOARD_HOST_DISPLAY_PROFILE})

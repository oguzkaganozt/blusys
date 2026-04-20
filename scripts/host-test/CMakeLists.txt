# scripts/host-test — minimal headless test loop (no SDL, no libmosquitto, no LVGL).
#
# A deliberately bare CMake project whose only purpose is to build and run
# the headless tests under scripts/host/src/*_smoke.cpp against the fake HAL
# and capability mocks. This exists as a sibling to scripts/host/ because
# the latter's interactive host bridge requires SDL2 and libmosquitto at
# configure time — which contradicts the fast iteration loop promise.
#
# To use:
#   cmake -S scripts/host-test -B build-host-test
#   cmake --build build-host-test
#   ctest --test-dir build-host-test --output-on-failure
#
# Adding a new test: list its sources, ensure it links only the libraries
# below, register via add_test. Do not reach for SDL/LVGL/libmosquitto here
# — if a test needs them, it belongs in scripts/host/ instead.

cmake_minimum_required(VERSION 3.16)
project(blusys_host_test LANGUAGES C CXX)

set(CMAKE_C_STANDARD            11)
set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

get_filename_component(BLUSYS_REPO_ROOT  "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(BLUSYS_COMPONENT_DIR "${BLUSYS_REPO_ROOT}/components/blusys")

# ── Minimal framework core (no FreeRTOS, no ESP-IDF, no SDL, no mosquitto) ──
add_library(blusys_test_core STATIC
    ${BLUSYS_COMPONENT_DIR}/src/hal/error.c
    ${BLUSYS_COMPONENT_DIR}/src/observe/error_domain.c
    ${BLUSYS_COMPONENT_DIR}/src/observe/log.c
    ${BLUSYS_COMPONENT_DIR}/src/observe/counter.c
    ${BLUSYS_COMPONENT_DIR}/src/observe/snapshot.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/feedback/feedback.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/feedback/presets.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/feedback/internal/logging_sink.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/events/framework_init.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/events/router.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/events/event_queue.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/app/ctx.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/capability_event_map.cpp
    ${BLUSYS_COMPONENT_DIR}/src/framework/test/test_stubs.cpp
)
target_include_directories(blusys_test_core PUBLIC
    ${BLUSYS_COMPONENT_DIR}/include
    ${BLUSYS_REPO_ROOT}/scripts/host/include_host
)
target_compile_definitions(blusys_test_core PUBLIC BLUSYS_DEVICE_BUILD=0)
target_compile_options(blusys_test_core PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    -Wall -Wextra
)

# ── Persistence host stub (uses STL containers; compiled without -fno-exceptions) ──
add_library(blusys_persistence_host STATIC
    ${BLUSYS_COMPONENT_DIR}/src/framework/capabilities/persistence_host.cpp
)
target_include_directories(blusys_persistence_host PUBLIC
    ${BLUSYS_COMPONENT_DIR}/include
    ${BLUSYS_REPO_ROOT}/scripts/host/include_host
)
target_link_libraries(blusys_persistence_host PUBLIC blusys_test_core)
target_compile_options(blusys_persistence_host PRIVATE -Wall -Wextra)

# ── Fake HAL backend (replaces real hal/gpio.c, hal/timer.c, hal/uart.c) ──
add_library(blusys_fake_hal STATIC
    ${BLUSYS_COMPONENT_DIR}/src/framework/test/fake_hal/gpio.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/test/fake_hal/timer.c
    ${BLUSYS_COMPONENT_DIR}/src/framework/test/fake_hal/uart.c
)
target_include_directories(blusys_fake_hal PUBLIC
    ${BLUSYS_COMPONENT_DIR}/include
)
target_compile_options(blusys_fake_hal PRIVATE -Wall -Wextra)

# ── host_harness_smoke ─────────────────────────────────────────────────
add_executable(host_harness_smoke ${BLUSYS_REPO_ROOT}/scripts/host/src/host_harness_smoke.cpp)
target_link_libraries(host_harness_smoke PRIVATE blusys_test_core blusys_persistence_host blusys_fake_hal)
target_compile_options(host_harness_smoke PRIVATE
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -Wall -Wextra
)
add_test(NAME host_harness_smoke COMMAND host_harness_smoke)

# ── session_helper_smoke (session state machine) ───────────────────────────
add_executable(session_helper_smoke ${BLUSYS_REPO_ROOT}/scripts/host/src/session_helper_smoke.cpp)
target_link_libraries(session_helper_smoke PRIVATE blusys_test_core blusys_persistence_host)
target_compile_options(session_helper_smoke PRIVATE
    -fno-exceptions -fno-rtti -fno-threadsafe-statics
    -Wall -Wextra
)
add_test(NAME session_helper_smoke COMMAND session_helper_smoke)

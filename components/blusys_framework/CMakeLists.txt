set(blusys_framework_sources
    src/core/controller.cpp
    src/core/feedback.cpp
    src/core/feedback_presets.cpp
    src/app/feedback_logging_sink.cpp
    src/core/framework.cpp
    src/core/router.cpp
    src/core/runtime.cpp
    src/app/app_ctx.cpp
)

# Capabilities — device-only (depend on blusys_services C APIs).
if(ESP_PLATFORM)
    list(APPEND blusys_framework_sources
        src/app/app_headless_platform_esp.cpp
        src/app/capabilities/connectivity.cpp
        src/app/capabilities/storage.cpp
        src/app/capabilities/bluetooth.cpp
        src/app/capabilities/ota.cpp
        src/app/capabilities/diagnostics.cpp
        src/app/capabilities/provisioning.cpp
        src/app/button_array_bridge.cpp
    )
endif()

# Telemetry capability — same implementation on device and host.
list(APPEND blusys_framework_sources
    src/app/capabilities/telemetry.cpp
)

idf_build_get_property(build_components BUILD_COMPONENTS)

if(BLUSYS_BUILD_UI)
    if("lvgl__lvgl" IN_LIST build_components)
        include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/blusys_framework_ui_sources.cmake")
        foreach(_ui_src IN LISTS BLUSYS_FRAMEWORK_UI_SRC_SHARED)
            list(APPEND blusys_framework_sources "${_ui_src}")
        endforeach()
        foreach(_ui_src IN LISTS BLUSYS_FRAMEWORK_UI_SRC_DEVICE_ONLY)
            list(APPEND blusys_framework_sources "${_ui_src}")
        endforeach()
    else()
        message(FATAL_ERROR "BLUSYS_BUILD_UI=ON requires the lvgl/lvgl managed component")
    endif()
endif()

idf_component_register(
    SRCS ${blusys_framework_sources}
    INCLUDE_DIRS include
    REQUIRES blusys blusys_services spi_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Os
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

if(BLUSYS_BUILD_UI)
    idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${lvgl_lib})
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BLUSYS_FRAMEWORK_HAS_UI=1)
endif()

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
        list(APPEND blusys_framework_sources
            src/app/theme_presets.cpp
            src/app/action_widgets.cpp
            src/app/bindings.cpp
            src/app/overlay_manager.cpp
            src/app/page.cpp
            src/app/screen_registry.cpp
            src/app/screen_router.cpp
            src/app/input_bridge.cpp
            src/app/app_device_platform.cpp
            src/ui/icons/icon_set_minimal.cpp
            src/ui/input/encoder.cpp
            src/ui/primitives/col.cpp
            src/ui/primitives/divider.cpp
            src/ui/primitives/label.cpp
            src/ui/primitives/row.cpp
            src/ui/primitives/screen.cpp
            src/ui/theme.cpp
            src/ui/visual_feedback.cpp
            src/ui/primitives/icon_label.cpp
            src/ui/primitives/status_badge.cpp
            src/ui/primitives/key_value.cpp
            src/ui/widgets/button/button.cpp
            src/ui/widgets/card/card.cpp
            src/ui/widgets/chart/chart.cpp
            src/ui/widgets/data_table/data_table.cpp
            src/ui/widgets/dropdown/dropdown.cpp
            src/ui/widgets/gauge/gauge.cpp
            src/ui/widgets/input_field/input_field.cpp
            src/ui/widgets/knob/knob.cpp
            src/ui/widgets/list/list.cpp
            src/ui/widgets/modal/modal.cpp
            src/ui/widgets/overlay/overlay.cpp
            src/ui/widgets/progress/progress.cpp
            src/ui/widgets/slider/slider.cpp
            src/ui/widgets/tabs/tabs.cpp
            src/ui/widgets/toggle/toggle.cpp
            src/app/shell.cpp
            src/app/touch_bridge.cpp
            src/ui/transition.cpp
            src/ui/input/focus_scope.cpp
            src/app/flows/boot.cpp
            src/app/flows/loading.cpp
            src/app/flows/error.cpp
            src/app/flows/status.cpp
            src/app/flows/settings.cpp
            src/app/flows/ota_flow.cpp
            src/app/flows/provisioning_flow.cpp
            src/app/flows/connectivity_flow.cpp
            src/app/flows/diagnostics_flow.cpp
            src/app/screens/about_screen.cpp
            src/app/screens/status_screen.cpp
            src/app/screens/diagnostics_screen.cpp
        )
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

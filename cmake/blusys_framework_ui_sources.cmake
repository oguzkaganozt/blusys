# Single source of truth for framework UI-related .cpp paths (relative to
# components/blusys_framework/). Used by:
#   - components/blusys_framework/CMakeLists.txt (ESP-IDF, BLUSYS_BUILD_UI)
#   - scripts/host/CMakeLists.txt (full host harness)
#   - cmake/blusys_framework_host_widgetkit.cmake (LVGL primitives/widgets only)
#
# ESP-IDF builds also compile two device-only translation units when UI is on;
# host builds omit them.

set(BLUSYS_FRAMEWORK_UI_SRC_DEVICE_ONLY
    src/app/input_bridge.cpp
    src/app/app_device_platform.cpp
)

# Full interactive UI bundle shared by device (minus device-only) and host harness.
set(BLUSYS_FRAMEWORK_UI_SRC_SHARED
    src/app/theme_presets.cpp
    src/app/action_widgets.cpp
    src/app/bindings.cpp
    src/app/overlay_manager.cpp
    src/app/page.cpp
    src/app/screen_registry.cpp
    src/app/screen_router.cpp
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
    src/ui/widgets/level_bar/level_bar.cpp
    src/ui/widgets/list/list.cpp
    src/ui/widgets/modal/modal.cpp
    src/ui/widgets/overlay/overlay.cpp
    src/ui/widgets/progress/progress.cpp
    src/ui/widgets/slider/slider.cpp
    src/ui/widgets/tabs/tabs.cpp
    src/ui/widgets/toggle/toggle.cpp
    src/ui/widgets/vu_strip/vu_strip.cpp
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

# Minimal host examples: only LVGL-facing sources (icons, input, primitives, widgets,
# transition, focus). Keep in sync with BLUSYS_FRAMEWORK_UI_SRC_SHARED src/ui/** entries.
set(BLUSYS_FRAMEWORK_HOST_WIDGETKIT_REL
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
    src/ui/widgets/level_bar/level_bar.cpp
    src/ui/widgets/list/list.cpp
    src/ui/widgets/modal/modal.cpp
    src/ui/widgets/overlay/overlay.cpp
    src/ui/widgets/progress/progress.cpp
    src/ui/widgets/slider/slider.cpp
    src/ui/widgets/tabs/tabs.cpp
    src/ui/widgets/toggle/toggle.cpp
    src/ui/widgets/vu_strip/vu_strip.cpp
    src/ui/transition.cpp
    src/ui/input/focus_scope.cpp
)

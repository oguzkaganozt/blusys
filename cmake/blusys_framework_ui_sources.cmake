# Single source of truth for framework UI-related .cpp paths (relative to
# components/blusys_framework/). Used by:
#   - components/blusys_framework/CMakeLists.txt (ESP-IDF, BLUSYS_BUILD_UI)
#   - scripts/host/CMakeLists.txt (full host harness)
#   - cmake/blusys_framework_host_widgetkit.cmake (LVGL primitives/widgets only)
#
# Widget translation units under src/ui/widgets/<name>/<name>.cpp (one directory
# level under widgets/) are auto-discovered so new widgets do not require editing
# long manual lists.
#
# ESP-IDF builds also compile two device-only translation units when UI is on;
# host builds omit them.

get_filename_component(_blusys_framework_root "${CMAKE_CURRENT_LIST_DIR}/../components/blusys_framework" ABSOLUTE)
file(GLOB _blusys_ui_widget_abs CONFIGURE_DEPENDS
    "${_blusys_framework_root}/src/ui/widgets/*/*.cpp")
list(SORT _blusys_ui_widget_abs)
set(BLUSYS_FRAMEWORK_UI_WIDGET_SRC_REL "")
foreach(_w IN LISTS _blusys_ui_widget_abs)
    file(RELATIVE_PATH _rel ${_blusys_framework_root} ${_w})
    list(APPEND BLUSYS_FRAMEWORK_UI_WIDGET_SRC_REL ${_rel})
endforeach()

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
    ${BLUSYS_FRAMEWORK_UI_WIDGET_SRC_REL}
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

# Minimal host examples: LVGL-facing sources (icons, input, primitives, widgets,
# transition, focus). Widget list matches BLUSYS_FRAMEWORK_UI_WIDGET_SRC_REL.
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
    ${BLUSYS_FRAMEWORK_UI_WIDGET_SRC_REL}
    src/ui/transition.cpp
    src/ui/input/focus_scope.cpp
)

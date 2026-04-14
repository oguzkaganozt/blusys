# LVGL primitives + widgets for minimal host example builds.
#
# Requires BLUSYS_PATH (the repo root) to be set before include.
# Produces BLUSYS_FRAMEWORK_HOST_WIDGETKIT — a list of absolute source paths
# covering style tokens, input, primitives, and the flat widget set.
# Does NOT include the full app spine, flows, capabilities, or screens.

if(NOT BLUSYS_PATH)
    message(FATAL_ERROR "blusys_framework_host_widgetkit.cmake: set BLUSYS_PATH to the blusys repo root before include")
endif()

set(_blusys_comp "${BLUSYS_PATH}/components/blusys")

set(BLUSYS_FRAMEWORK_HOST_WIDGETKIT
    # style
    ${_blusys_comp}/src/framework/ui/style/icon_set_minimal.cpp
    ${_blusys_comp}/src/framework/ui/style/theme.cpp
    ${_blusys_comp}/src/framework/ui/style/interaction_effects.cpp
    ${_blusys_comp}/src/framework/ui/style/transition.cpp
    # input
    ${_blusys_comp}/src/framework/ui/input/encoder.cpp
    ${_blusys_comp}/src/framework/ui/input/focus_scope.cpp
    # primitives
    ${_blusys_comp}/src/framework/ui/primitives/col.cpp
    ${_blusys_comp}/src/framework/ui/primitives/divider.cpp
    ${_blusys_comp}/src/framework/ui/primitives/icon_label.cpp
    ${_blusys_comp}/src/framework/ui/primitives/key_value.cpp
    ${_blusys_comp}/src/framework/ui/primitives/label.cpp
    ${_blusys_comp}/src/framework/ui/primitives/row.cpp
    ${_blusys_comp}/src/framework/ui/primitives/screen.cpp
    ${_blusys_comp}/src/framework/ui/primitives/status_badge.cpp
    # widgets (flat)
    ${_blusys_comp}/src/framework/ui/widgets/button.cpp
    ${_blusys_comp}/src/framework/ui/widgets/card.cpp
    ${_blusys_comp}/src/framework/ui/widgets/chart.cpp
    ${_blusys_comp}/src/framework/ui/widgets/data_table.cpp
    ${_blusys_comp}/src/framework/ui/widgets/dropdown.cpp
    ${_blusys_comp}/src/framework/ui/widgets/gauge.cpp
    ${_blusys_comp}/src/framework/ui/widgets/input_field.cpp
    ${_blusys_comp}/src/framework/ui/widgets/knob.cpp
    ${_blusys_comp}/src/framework/ui/widgets/level_bar.cpp
    ${_blusys_comp}/src/framework/ui/widgets/list.cpp
    ${_blusys_comp}/src/framework/ui/widgets/modal.cpp
    ${_blusys_comp}/src/framework/ui/widgets/overlay.cpp
    ${_blusys_comp}/src/framework/ui/widgets/progress.cpp
    ${_blusys_comp}/src/framework/ui/widgets/slider.cpp
    ${_blusys_comp}/src/framework/ui/widgets/tabs.cpp
    ${_blusys_comp}/src/framework/ui/widgets/toggle.cpp
    ${_blusys_comp}/src/framework/ui/widgets/vu_strip.cpp
)

unset(_blusys_comp)

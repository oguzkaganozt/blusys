# Shared LVGL widget + primitive sources for host builds that link
# components/blusys_framework/src/app/bindings.cpp:
#   - examples/*/host/CMakeLists.txt (interactive quickstarts / references)
#   - scripts/host/CMakeLists.txt (adds widgets/knob/knob.cpp on top for widget_kit_demo)
#
# Requires BLUSYS_FW = …/components/blusys_framework before include.
# When bindings.cpp gains new blusys::ui::* calls, add the matching widget .cpp here
# (and verify components/blusys_framework/CMakeLists.txt on the device side).

set(BLUSYS_FRAMEWORK_HOST_WIDGETKIT
    ${BLUSYS_FW}/src/ui/icons/icon_set_minimal.cpp
    ${BLUSYS_FW}/src/ui/input/encoder.cpp
    ${BLUSYS_FW}/src/ui/input/focus_scope.cpp
    ${BLUSYS_FW}/src/ui/primitives/col.cpp
    ${BLUSYS_FW}/src/ui/primitives/divider.cpp
    ${BLUSYS_FW}/src/ui/primitives/icon_label.cpp
    ${BLUSYS_FW}/src/ui/primitives/key_value.cpp
    ${BLUSYS_FW}/src/ui/primitives/label.cpp
    ${BLUSYS_FW}/src/ui/primitives/row.cpp
    ${BLUSYS_FW}/src/ui/primitives/screen.cpp
    ${BLUSYS_FW}/src/ui/primitives/status_badge.cpp
    ${BLUSYS_FW}/src/ui/theme.cpp
    ${BLUSYS_FW}/src/ui/visual_feedback.cpp
    ${BLUSYS_FW}/src/ui/transition.cpp
    ${BLUSYS_FW}/src/ui/widgets/button/button.cpp
    ${BLUSYS_FW}/src/ui/widgets/card/card.cpp
    ${BLUSYS_FW}/src/ui/widgets/chart/chart.cpp
    ${BLUSYS_FW}/src/ui/widgets/data_table/data_table.cpp
    ${BLUSYS_FW}/src/ui/widgets/dropdown/dropdown.cpp
    ${BLUSYS_FW}/src/ui/widgets/gauge/gauge.cpp
    ${BLUSYS_FW}/src/ui/widgets/input_field/input_field.cpp
    ${BLUSYS_FW}/src/ui/widgets/level_bar/level_bar.cpp
    ${BLUSYS_FW}/src/ui/widgets/list/list.cpp
    ${BLUSYS_FW}/src/ui/widgets/modal/modal.cpp
    ${BLUSYS_FW}/src/ui/widgets/overlay/overlay.cpp
    ${BLUSYS_FW}/src/ui/widgets/progress/progress.cpp
    ${BLUSYS_FW}/src/ui/widgets/slider/slider.cpp
    ${BLUSYS_FW}/src/ui/widgets/tabs/tabs.cpp
    ${BLUSYS_FW}/src/ui/widgets/toggle/toggle.cpp
    ${BLUSYS_FW}/src/ui/widgets/vu_strip/vu_strip.cpp
)

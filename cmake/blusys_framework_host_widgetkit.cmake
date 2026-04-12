# LVGL primitives + widgets for minimal host example builds (see
# examples/quickstart/interactive_controller/host/CMakeLists.txt).
#
# Requires BLUSYS_FW = absolute path to components/blusys_framework before include.
# Paths are derived from cmake/blusys_framework_ui_sources.cmake (single source of truth).

if(NOT BLUSYS_FW)
    message(FATAL_ERROR "blusys_framework_host_widgetkit.cmake: set BLUSYS_FW to components/blusys_framework before include")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/blusys_framework_ui_sources.cmake")

set(BLUSYS_FRAMEWORK_HOST_WIDGETKIT "")
foreach(_r IN LISTS BLUSYS_FRAMEWORK_HOST_WIDGETKIT_REL)
    list(APPEND BLUSYS_FRAMEWORK_HOST_WIDGETKIT "${BLUSYS_FW}/${_r}")
endforeach()

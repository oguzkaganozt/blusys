set(_edge_main_srcs
    "logic/app_logic.cpp"
    "system/app_main.cpp"
)
if(BLUSYS_BUILD_UI)
    list(APPEND _edge_main_srcs "ui/app_ui.cpp")
endif()

idf_component_register(
    SRCS ${_edge_main_srcs}
    INCLUDE_DIRS "."
    REQUIRES blusys_framework
)

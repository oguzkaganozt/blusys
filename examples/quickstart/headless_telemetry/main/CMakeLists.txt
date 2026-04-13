set(_ht_main_srcs
    "core/app_logic.cpp"
    "integration/app_main.cpp"
)
if(BLUSYS_BUILD_UI)
    list(APPEND _ht_main_srcs "ui/app_ui.cpp")
endif()

idf_component_register(
    SRCS ${_ht_main_srcs}
    INCLUDE_DIRS "."
    REQUIRES blusys_framework
)

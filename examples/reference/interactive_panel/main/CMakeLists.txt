idf_component_register(
    SRCS
        "logic/app_logic.cpp"
        "ui/app_ui.cpp"
        "system/app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES blusys_framework
)

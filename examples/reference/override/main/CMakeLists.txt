idf_component_register(
    SRCS
        "platform/app_main.cpp"
        "platform/override_capability.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES blusys
)

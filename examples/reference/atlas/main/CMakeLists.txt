idf_component_register(
    SRCS
        "core/atlas_logic.cpp"
        "platform/atlas_capability.cpp"
        "platform/atlas_main.cpp"
    INCLUDE_DIRS
        "."
        "include"
    REQUIRES blusys
)

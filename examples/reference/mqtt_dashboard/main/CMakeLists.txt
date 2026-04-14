idf_component_register(
    SRCS
        "core/app_logic.cpp"
        "ui/app_ui.cpp"
        "platform/app_main.cpp"
        "platform/mqtt_esp_capability.cpp"
    INCLUDE_DIRS
        "."
        "../../../include"
    REQUIRES blusys esp_netif
)

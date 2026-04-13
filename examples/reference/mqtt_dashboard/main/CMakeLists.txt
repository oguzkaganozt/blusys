idf_component_register(
    SRCS
        "core/app_logic.cpp"
        "ui/app_ui.cpp"
        "integration/app_main.cpp"
        "integration/mqtt_esp_capability.cpp"
    INCLUDE_DIRS
        "."
        "../../../include"
    REQUIRES blusys_framework esp_netif
)

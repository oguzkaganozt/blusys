idf_component_register(
    SRCS
        "connectivity_main.c"
        "ref_connectivity_wifi.c"
        "ref_connectivity_http.c"
        "ref_connectivity_mqtt.c"
    REQUIRES blusys
)

idf_component_register(
    SRCS
        "network_services_main.c"
        "net_http_server.c"
        "net_ws_client.c"
        "net_mdns.c"
        "net_sntp.c"
        "net_ota.c"
        "net_wifi_prov.c"
        "net_local_ctrl.c"
    REQUIRES blusys
)

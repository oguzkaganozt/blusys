set(blusys_services_sources
    src/ble_gatt.c
    src/bluetooth.c
    src/button.c
    src/console.c
    src/encoder.c
    src/espnow.c
    src/fatfs.c
    src/fs.c
    src/http_client.c
    src/http_server.c
    src/lcd.c
    src/led_strip.c
    src/mdns.c
    src/mqtt.c
    src/ota.c
    src/power_mgmt.c
    src/sntp.c
    src/wifi.c
    src/wifi_prov.c
    src/ws_client.c
)

idf_component_register(
    SRCS ${blusys_services_sources}
    INCLUDE_DIRS include
    REQUIRES blusys freertos console esp_wifi esp_netif esp_event
             esp_http_client esp_http_server mqtt esp_https_ota app_update
             bt spiffs fatfs wear_levelling esp_pm tcp_transport
             wifi_provisioning protocomm esp_lcd esp_driver_gpio
             esp_driver_spi esp_driver_i2c esp_driver_rmt esp_timer nvs_flash
)

# Optional managed component: espressif/mdns
# Only available when the project declares espressif/mdns in its idf_component.yml.
idf_build_get_property(build_components BUILD_COMPONENTS)
if("mdns" IN_LIST build_components)
    idf_component_get_property(mdns_lib mdns COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${mdns_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_MDNS=1)
endif()

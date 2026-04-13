set(blusys_services_sources
    # display
    src/display/ui.c
    # input
    src/input/usb_hid.c
    # connectivity
    src/connectivity/ble_gatt.c
    src/connectivity/bluetooth.c
    src/connectivity/espnow.c
    src/connectivity/mdns.c
    src/connectivity/wifi.c
    src/connectivity/wifi_mesh.c
    src/connectivity/wifi_prov.c
    # protocol
    src/protocol/http_client.c
    src/protocol/http_server.c
    src/protocol/mqtt.c
    src/protocol/ws_client.c
    # device
    src/device/console.c
    src/device/fatfs.c
    src/device/fs.c
    src/device/local_ctrl.c
    src/device/ota.c
    src/device/power_mgmt.c
    src/device/sntp.c
)

idf_component_register(
    SRCS ${blusys_services_sources}
    INCLUDE_DIRS include
    REQUIRES blusys_hal freertos console esp_wifi esp_netif esp_event
             esp_http_client esp_http_server mqtt esp_https_ota app_update
             bt spiffs fatfs wear_levelling esp_pm tcp_transport
             wifi_provisioning protocomm esp_timer nvs_flash
)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/blusys_optional_component.cmake")

# Optional managed component: espressif/mdns
# Only available when the project declares espressif/mdns in its idf_component.yml.
idf_build_get_property(build_components BUILD_COMPONENTS)
blusys_optional_managed_component(blusys_mdns_component "mdns" "espressif__mdns")

if(NOT blusys_mdns_component STREQUAL "")
    idf_component_get_property(mdns_lib ${blusys_mdns_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${mdns_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_MDNS=1)
endif()

# Optional managed component: espressif/usb_host_hid
# Only available when the project declares espressif/usb_host_hid in its idf_component.yml.
blusys_optional_managed_component(blusys_usb_host_hid_component "usb_host_hid" "espressif__usb_host_hid")

if(NOT blusys_usb_host_hid_component STREQUAL "")
    idf_component_get_property(usb_host_hid_lib ${blusys_usb_host_hid_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${usb_host_hid_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_USB_HOST_HID=1)
endif()

# Optional managed component: lvgl/lvgl
# Only available when the project declares lvgl/lvgl in its idf_component.yml.
# The component registers as "lvgl__lvgl" in the build system.
if("lvgl__lvgl" IN_LIST build_components)
    idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${lvgl_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_LVGL=1)
endif()

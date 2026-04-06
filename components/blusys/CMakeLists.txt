set(blusys_sources
    src/common/adc.c
    src/common/ble_gatt.c
    src/common/button.c
    src/common/dac.c
    src/common/error.c
    src/common/espnow.c
    src/common/fs.c
    src/common/gpio.c
    src/common/i2c.c
    src/common/i2c_slave.c
    src/common/i2s.c
    src/common/i2s_rx.c
    src/common/mcpwm.c
    src/common/pcnt.c
    src/common/pwm.c
    src/common/rmt.c
    src/common/rmt_rx.c
    src/common/sdm.c
    src/common/sdmmc.c
    src/common/sleep.c
    src/common/spi.c
    src/common/spi_slave.c
    src/common/system.c
    src/common/temp_sensor.c
    src/common/timer.c
    src/common/touch.c
    src/common/target.c
    src/common/twai.c
    src/common/uart.c
    src/common/version.c
    src/common/wdt.c
    src/common/http_client.c
    src/common/http_server.c
    src/common/ota.c
    src/common/mqtt.c
    src/common/nvs.c
    src/common/sntp.c
    src/common/bluetooth.c
    src/common/mdns.c
    src/common/wifi.c
    src/internal/blusys_lock_freertos.c
)

if(IDF_TARGET STREQUAL "esp32")
    list(APPEND blusys_sources src/targets/esp32/target_caps.c)
elseif(IDF_TARGET STREQUAL "esp32c3")
    list(APPEND blusys_sources src/targets/esp32c3/target_caps.c)
elseif(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND blusys_sources src/targets/esp32s3/target_caps.c)
else()
    message(FATAL_ERROR "blusys currently supports only esp32, esp32c3, and esp32s3")
endif()

idf_component_register(
    SRCS ${blusys_sources}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS src/internal
    REQUIRES freertos esp_adc esp_driver_dac esp_driver_gpio esp_driver_gptimer esp_driver_i2c esp_driver_i2s esp_driver_ledc esp_driver_mcpwm esp_driver_pcnt esp_driver_rmt esp_driver_sdm esp_driver_sdmmc esp_driver_spi esp_driver_touch_sens esp_driver_tsens esp_driver_twai esp_driver_uart esp_system esp_timer esp_wifi esp_netif esp_event nvs_flash esp_http_client esp_http_server mqtt esp_https_ota app_update bt spiffs
)

# Optional managed component: espressif/mdns
# Only available when the project declares espressif/mdns in its idf_component.yml.
idf_build_get_property(build_components BUILD_COMPONENTS)
if("mdns" IN_LIST build_components)
    idf_component_get_property(mdns_lib mdns COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${mdns_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_MDNS=1)
endif()

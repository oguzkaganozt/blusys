set(blusys_sources
    src/common/adc.c
    src/common/bt_stack.c
    src/common/dac.c
    src/common/efuse.c
    src/common/error.c
    src/common/gpio.c
    src/common/gpio_expander.c
    src/common/i2c.c
    src/common/i2c_slave.c
    src/common/i2s.c
    src/common/i2s_rx.c
    src/common/mcpwm.c
    src/common/nvs.c
    src/common/one_wire.c
    src/common/pcnt.c
    src/common/pwm.c
    src/common/rmt.c
    src/common/rmt_rx.c
    src/common/sd_spi.c
    src/common/sdm.c
    src/common/sdmmc.c
    src/common/sleep.c
    src/common/spi.c
    src/common/spi_slave.c
    src/common/system.c
    src/common/target.c
    src/common/temp_sensor.c
    src/common/timer.c
    src/common/touch.c
    src/common/twai.c
    src/common/ulp.c
    src/common/uart.c
    src/common/version.c
    src/common/usb_device.c
    src/common/usb_host.c
    src/common/wdt.c
    src/drivers/display/lcd.c
    src/drivers/display/lcd_panel_ili.c
    src/drivers/display/led_strip.c
    src/drivers/display/seven_seg.c
    src/drivers/input/button.c
    src/drivers/input/encoder.c
    src/drivers/sensor/dht.c
    src/drivers/actuator/buzzer.c
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
    REQUIRES freertos efuse esp_adc esp_driver_dac esp_driver_gpio esp_driver_gptimer
             esp_driver_i2c esp_driver_i2s esp_driver_ledc esp_driver_mcpwm
             esp_driver_pcnt esp_driver_rmt esp_driver_sdm esp_driver_sdmmc
             esp_driver_sdspi esp_driver_spi esp_driver_touch_sens esp_driver_tsens esp_driver_twai
             esp_driver_uart esp_hw_support esp_system esp_timer bootloader_support fatfs nvs_flash sdmmc ulp usb esp_lcd
)

# Optional managed component: espressif/esp_tinyusb
# Only available when the project declares espressif/esp_tinyusb in its idf_component.yml.
idf_build_get_property(build_components BUILD_COMPONENTS)
set(blusys_tinyusb_component "")
if("esp_tinyusb" IN_LIST build_components)
    set(blusys_tinyusb_component esp_tinyusb)
elseif("espressif__esp_tinyusb" IN_LIST build_components)
    set(blusys_tinyusb_component espressif__esp_tinyusb)
endif()

if(NOT blusys_tinyusb_component STREQUAL "")
    idf_component_get_property(tinyusb_lib ${blusys_tinyusb_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${tinyusb_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_TINYUSB=1)
endif()

if(CONFIG_ULP_COPROC_ENABLED AND CONFIG_ULP_COPROC_TYPE_FSM AND
   (IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32s3"))
    set(blusys_ulp_app_name blusys_ulp_gpio_watch)
    set(blusys_ulp_sources
        "src/ulp/gpio_watch.S"
        "src/ulp/wake_up.S"
    )
    set(blusys_ulp_dep_sources "src/common/ulp.c")
    ulp_embed_binary(${blusys_ulp_app_name}
                     "${blusys_ulp_sources}"
                     "${blusys_ulp_dep_sources}"
                     PREFIX "blusys_ulp_gpio_watch_")
endif()

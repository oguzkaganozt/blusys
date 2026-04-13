set(blusys_sources
    src/soc/adc.c
    src/soc/dac.c
    src/soc/gpio.c
    src/soc/gpio_expander.c
    src/soc/i2c.c
    src/soc/i2c_slave.c
    src/soc/i2s.c
    src/soc/i2s_rx.c
    src/soc/mcpwm.c
    src/soc/one_wire.c
    src/soc/pcnt.c
    src/soc/pwm.c
    src/soc/rmt.c
    src/soc/rmt_rx.c
    src/soc/sd_spi.c
    src/soc/sdm.c
    src/soc/sdmmc.c
    src/soc/spi.c
    src/soc/spi_slave.c
    src/soc/timer.c
    src/soc/touch.c
    src/soc/twai.c
    src/soc/uart.c
    src/soc/temp_sensor.c
    src/platform/error.c
    src/platform/version.c
    src/platform/nvs.c
    src/platform/sleep.c
    src/platform/system.c
    src/platform/target.c
    src/platform/wdt.c
    src/platform/efuse.c
    src/usb/usb_device.c
    src/usb/usb_host.c
    src/wireless/bt_stack.c
    src/rt/blusys_lock_freertos.c
    src/drivers/display/lcd.c
    src/drivers/display/lcd_panel_ili.c
    src/drivers/display/led_strip.c
    src/drivers/display/seven_seg.c
    src/drivers/input/button.c
    src/drivers/input/encoder.c
    src/drivers/sensor/dht.c
    src/drivers/actuator/buzzer.c
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

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/blusys_optional_component.cmake")

# Optional managed component: espressif/esp_tinyusb
# Only available when the project declares espressif/esp_tinyusb in its idf_component.yml.
idf_build_get_property(build_components BUILD_COMPONENTS)
blusys_optional_managed_component(blusys_tinyusb_component "esp_tinyusb" "espressif__esp_tinyusb")

if(NOT blusys_tinyusb_component STREQUAL "")
    idf_component_get_property(tinyusb_lib ${blusys_tinyusb_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${tinyusb_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_TINYUSB=1)
endif()

blusys_optional_managed_component(blusys_esp_lcd_qemu_rgb "esp_lcd_qemu_rgb" "espressif__esp_lcd_qemu_rgb")

if(NOT blusys_esp_lcd_qemu_rgb STREQUAL "")
    idf_component_get_property(esp_lcd_qemu_rgb_lib ${blusys_esp_lcd_qemu_rgb} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${esp_lcd_qemu_rgb_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_ESP_LCD_QEMU_RGB=1)
endif()

if(CONFIG_ULP_COPROC_ENABLED AND CONFIG_ULP_COPROC_TYPE_FSM AND
   (IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32s3"))
    set(blusys_ulp_app_name blusys_ulp_gpio_watch)
    set(blusys_ulp_sources
        "src/ulp/gpio_watch.S"
        "src/ulp/wake_up.S"
    )
    set(blusys_ulp_dep_sources "src/ulp/ulp.c")
    ulp_embed_binary(${blusys_ulp_app_name}
                     "${blusys_ulp_sources}"
                     "${blusys_ulp_dep_sources}"
                     PREFIX "blusys_ulp_gpio_watch_")
endif()

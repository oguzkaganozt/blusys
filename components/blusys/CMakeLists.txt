set(blusys_sources
    src/common/adc.c
    src/common/error.c
    src/common/gpio.c
    src/common/i2c.c
    src/common/pwm.c
    src/common/spi.c
    src/common/system.c
    src/common/timer.c
    src/common/target.c
    src/common/uart.c
    src/common/version.c
    src/internal/blusys_lock_freertos.c
)

if(IDF_TARGET STREQUAL "esp32c3")
    list(APPEND blusys_sources src/targets/esp32c3/target_caps.c)
elseif(IDF_TARGET STREQUAL "esp32s3")
    list(APPEND blusys_sources src/targets/esp32s3/target_caps.c)
else()
    message(FATAL_ERROR "blusys currently supports only esp32c3 and esp32s3")
endif()

idf_component_register(
    SRCS ${blusys_sources}
    INCLUDE_DIRS include
    PRIV_INCLUDE_DIRS src/internal
    REQUIRES freertos esp_adc esp_driver_gpio esp_driver_gptimer esp_driver_i2c esp_driver_ledc esp_driver_spi esp_driver_uart esp_system esp_timer
)

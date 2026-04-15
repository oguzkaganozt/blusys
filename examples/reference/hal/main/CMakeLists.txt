idf_component_register(
    SRCS
        "hal_reference_main.c"
        "ref_hal_uart.c"
        "ref_hal_gpio_basic.c"
        "ref_hal_pwm_basic.c"
        "ref_hal_button_basic.c"
        "ref_hal_timer_basic.c"
        "ref_hal_nvs_basic.c"
        "ref_hal_adc_basic.c"
        "ref_hal_spi_loopback.c"
        "ref_hal_i2c_scan.c"
    REQUIRES blusys
)

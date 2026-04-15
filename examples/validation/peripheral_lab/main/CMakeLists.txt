idf_component_register(
    SRCS
        "peripheral_lab_main.c"
        "periph_i2c_slave.c"
        "periph_spi_slave.c"
        "periph_twai.c"
        "periph_rmt.c"
        "periph_i2s.c"
        "periph_sdmmc.c"
        "periph_sd_spi.c"
        "periph_gpio_expander.c"
        "periph_lcd_ssd1306.c"
    REQUIRES blusys
)

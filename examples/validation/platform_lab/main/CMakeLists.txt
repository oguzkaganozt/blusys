idf_component_register(
    SRCS
        "platform_lab_main.c"
        "plat_smoke.c"
        "plat_storage.c"
        "plat_power_mgmt.c"
        "plat_console.c"
        "plat_framework_core.cpp"
        "plat_concurrency_main.c"
        "suite_timer.c"
        "suite_i2c.c"
        "suite_spi.c"
        "suite_uart.c"
    REQUIRES blusys
)

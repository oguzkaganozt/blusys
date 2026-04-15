#if CONFIG_PLATFORM_LAB_SCENARIO_CONCURRENCY
#include "sdkconfig.h"

void run_concurrency_timer_suite(void);
void run_concurrency_i2c_suite(void);
void run_concurrency_spi_suite(void);
void run_concurrency_uart_suite(void);

void run_platform_concurrency(void)
{
#if defined(CONFIG_BLUSYS_CONCURRENCY_SUITE_TIMER)
    run_concurrency_timer_suite();
#elif defined(CONFIG_BLUSYS_CONCURRENCY_SUITE_I2C)
    run_concurrency_i2c_suite();
#elif defined(CONFIG_BLUSYS_CONCURRENCY_SUITE_SPI)
    run_concurrency_spi_suite();
#elif defined(CONFIG_BLUSYS_CONCURRENCY_SUITE_UART)
    run_concurrency_uart_suite();
#endif
}

#else
void run_platform_concurrency(void) {}
#endif

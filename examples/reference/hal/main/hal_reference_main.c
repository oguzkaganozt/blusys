#include "sdkconfig.h"

void run_gpio_basic(void);
void run_pwm_basic(void);
void run_button_basic(void);
void run_timer_basic(void);
void run_nvs_basic(void);
void run_adc_basic(void);
void run_spi_loopback(void);
void run_i2c_scan(void);
void run_uart_basic(void);

void app_main(void)
{
#if CONFIG_REF_HAL_REFERENCE_SCENARIO_GPIO_BASIC
    run_gpio_basic();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_PWM_BASIC
    run_pwm_basic();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_BUTTON_BASIC
    run_button_basic();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_TIMER_BASIC
    run_timer_basic();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_NVS_BASIC
    run_nvs_basic();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_ADC_BASIC
    run_adc_basic();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_SPI_LOOPBACK
    run_spi_loopback();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_I2C_SCAN
    run_i2c_scan();
#elif CONFIG_REF_HAL_REFERENCE_SCENARIO_UART_BASIC
    run_uart_basic();
#endif
}

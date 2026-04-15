#include "sdkconfig.h"


void run_gpio_interrupt(void);
void run_pcnt_basic(void);
void run_mcpwm_basic(void);
void run_sdm_basic(void);
void run_dac_basic(void);
void run_led_strip_basic(void);
void run_seven_seg_basic(void);
void run_buzzer_basic(void);
void run_dht_basic(void);
void run_temp_sensor_basic(void);
void run_one_wire_basic(void);
void run_touch_basic(void);
void run_ulp_gpio_watch(void);
void run_efuse_info(void);
void run_wdt_basic(void);
void run_sleep_basic(void);
void run_system_info(void);

void app_main(void)
{
#if CONFIG_HAL_IO_LAB_SCENARIO_GPIO_INTERRUPT
    run_gpio_interrupt();
#elif CONFIG_HAL_IO_LAB_SCENARIO_PCNT_BASIC
    run_pcnt_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_MCPWM_BASIC
    run_mcpwm_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_SDM_BASIC
    run_sdm_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_DAC_BASIC
    run_dac_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_LED_STRIP_BASIC
    run_led_strip_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_SEVEN_SEG_BASIC
    run_seven_seg_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_BUZZER_BASIC
    run_buzzer_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_DHT_BASIC
    run_dht_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_TEMP_SENSOR_BASIC
    run_temp_sensor_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_ONE_WIRE_BASIC
    run_one_wire_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_TOUCH_BASIC
    run_touch_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_ULP_GPIO_WATCH
    run_ulp_gpio_watch();
#elif CONFIG_HAL_IO_LAB_SCENARIO_EFUSE_INFO
    run_efuse_info();
#elif CONFIG_HAL_IO_LAB_SCENARIO_WDT_BASIC
    run_wdt_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_SLEEP_BASIC
    run_sleep_basic();
#elif CONFIG_HAL_IO_LAB_SCENARIO_SYSTEM_INFO
    run_system_info();
#endif
}

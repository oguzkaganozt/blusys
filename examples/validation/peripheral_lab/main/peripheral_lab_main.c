#include "sdkconfig.h"

void run_periph_i2c_slave(void);
void run_periph_spi_slave(void);
void run_periph_twai(void);
void run_periph_rmt(void);
void run_periph_i2s(void);
void run_periph_sdmmc(void);
void run_periph_sd_spi(void);
void run_periph_gpio_expander(void);
void run_periph_lcd_ssd1306(void);

void app_main(void)
{
#if CONFIG_PERIPHERAL_LAB_SCENARIO_I2C_SLAVE
    run_periph_i2c_slave();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_SPI_SLAVE
    run_periph_spi_slave();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_TWAI
    run_periph_twai();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_RMT
    run_periph_rmt();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_I2S
    run_periph_i2s();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_SDMMC
    run_periph_sdmmc();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_SD_SPI
    run_periph_sd_spi();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_GPIO_EXPANDER
    run_periph_gpio_expander();
#elif CONFIG_PERIPHERAL_LAB_SCENARIO_LCD_SSD1306
    run_periph_lcd_ssd1306();
#endif
}

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/drivers/seven_seg.h"

static blusys_seven_seg_t *s_display;

static void open_display(void)
{
    blusys_seven_seg_config_t cfg = { 0 };
    blusys_err_t err;

#if defined(CONFIG_EXAMPLE_DRIVER_GPIO)
    cfg.driver      = BLUSYS_SEVEN_SEG_DRIVER_GPIO;
    cfg.digit_count = CONFIG_EXAMPLE_GPIO_DIGIT_COUNT;
    cfg.gpio = (blusys_seven_seg_gpio_config_t) {
        .pin_a        = CONFIG_EXAMPLE_GPIO_PIN_A,
        .pin_b        = CONFIG_EXAMPLE_GPIO_PIN_B,
        .pin_c        = CONFIG_EXAMPLE_GPIO_PIN_C,
        .pin_d        = CONFIG_EXAMPLE_GPIO_PIN_D,
        .pin_e        = CONFIG_EXAMPLE_GPIO_PIN_E,
        .pin_f        = CONFIG_EXAMPLE_GPIO_PIN_F,
        .pin_g        = CONFIG_EXAMPLE_GPIO_PIN_G,
        .pin_dp       = CONFIG_EXAMPLE_GPIO_PIN_DP,
        .common_pins  = {
            CONFIG_EXAMPLE_GPIO_COMMON0,
            CONFIG_EXAMPLE_GPIO_COMMON1,
            CONFIG_EXAMPLE_GPIO_COMMON2,
            CONFIG_EXAMPLE_GPIO_COMMON3,
            -1, -1, -1, -1,
        },
        .common_anode = CONFIG_EXAMPLE_GPIO_COMMON_ANODE,
    };

#elif defined(CONFIG_EXAMPLE_DRIVER_74HC595)
    cfg.driver      = BLUSYS_SEVEN_SEG_DRIVER_74HC595;
    cfg.digit_count = CONFIG_EXAMPLE_595_DIGIT_COUNT;
    cfg.hc595 = (blusys_seven_seg_595_config_t) {
        .data_pin    = CONFIG_EXAMPLE_595_DATA_PIN,
        .clk_pin     = CONFIG_EXAMPLE_595_CLK_PIN,
        .latch_pin   = CONFIG_EXAMPLE_595_LATCH_PIN,
        .common_pins = {
            CONFIG_EXAMPLE_595_COMMON0,
            CONFIG_EXAMPLE_595_COMMON1,
            CONFIG_EXAMPLE_595_COMMON2,
            CONFIG_EXAMPLE_595_COMMON3,
            -1, -1, -1, -1,
        },
        .common_anode = CONFIG_EXAMPLE_595_COMMON_ANODE,
    };

#else /* MAX7219 */
    cfg.driver      = BLUSYS_SEVEN_SEG_DRIVER_MAX7219;
    cfg.digit_count = CONFIG_EXAMPLE_MAX_DIGIT_COUNT;
    cfg.max7219 = (blusys_seven_seg_max7219_config_t) {
        .mosi_pin = CONFIG_EXAMPLE_MAX_MOSI_PIN,
        .clk_pin  = CONFIG_EXAMPLE_MAX_CLK_PIN,
        .cs_pin   = CONFIG_EXAMPLE_MAX_CS_PIN,
    };
#endif

    err = blusys_seven_seg_open(&cfg, &s_display);
    if (err != BLUSYS_OK) {
        printf("seven_seg_open failed: %d\n", err);
        return;
    }

    printf("7-segment display opened\n");
}

void app_main(void)
{
    open_display();
    if (s_display == NULL) {
        return;
    }

    int value = 0;
    while (1) {
        blusys_err_t err = blusys_seven_seg_write_int(s_display, value, false);
        if (err == BLUSYS_OK) {
            printf("display: %d\n", value);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
        value = (value >= 9999) ? 0 : value + 1;
    }
}

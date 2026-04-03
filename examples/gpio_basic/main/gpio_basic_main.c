#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_GPIO_BASIC_OUTPUT_PIN CONFIG_BLUSYS_GPIO_BASIC_OUTPUT_PIN
#define BLUSYS_GPIO_BASIC_INPUT_PIN CONFIG_BLUSYS_GPIO_BASIC_INPUT_PIN

static bool configure_pin(blusys_err_t err, const char *step)
{
    if (err == BLUSYS_OK) {
        return true;
    }

    printf("%s error: %s\n", step, blusys_err_string(err));
    return false;
}

void app_main(void)
{
    bool output_level = false;
    bool input_level = false;

    printf("Blusys gpio basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("output_pin: %d\n", BLUSYS_GPIO_BASIC_OUTPUT_PIN);
    printf("input_pin: %d\n", BLUSYS_GPIO_BASIC_INPUT_PIN);
    printf("set board-safe pins in menuconfig if these defaults do not match your board\n");

    if (!configure_pin(blusys_gpio_reset(BLUSYS_GPIO_BASIC_OUTPUT_PIN), "reset output pin") ||
        !configure_pin(blusys_gpio_set_output(BLUSYS_GPIO_BASIC_OUTPUT_PIN), "set output pin") ||
        !configure_pin(blusys_gpio_write(BLUSYS_GPIO_BASIC_OUTPUT_PIN, false), "initialize output level") ||
        !configure_pin(blusys_gpio_reset(BLUSYS_GPIO_BASIC_INPUT_PIN), "reset input pin") ||
        !configure_pin(blusys_gpio_set_input(BLUSYS_GPIO_BASIC_INPUT_PIN), "set input pin") ||
        !configure_pin(blusys_gpio_set_pull_mode(BLUSYS_GPIO_BASIC_INPUT_PIN, BLUSYS_GPIO_PULL_UP), "set input pull mode")) {
        return;
    }

    while (true) {
        output_level = !output_level;

        if (!configure_pin(blusys_gpio_write(BLUSYS_GPIO_BASIC_OUTPUT_PIN, output_level), "write output pin") ||
            !configure_pin(blusys_gpio_read(BLUSYS_GPIO_BASIC_INPUT_PIN, &input_level), "read input pin")) {
            return;
        }

        printf("output=%d input=%d\n", output_level ? 1 : 0, input_level ? 1 : 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

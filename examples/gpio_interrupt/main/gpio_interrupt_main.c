#include <stdio.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_GPIO_INTERRUPT_INPUT_PIN CONFIG_BLUSYS_GPIO_INTERRUPT_INPUT_PIN

typedef struct {
    TaskHandle_t task;
    volatile uint32_t interrupt_count;
} blusys_gpio_interrupt_ctx_t;

static bool blusys_gpio_interrupt_on_edge(int pin, void *user_ctx)
{
    blusys_gpio_interrupt_ctx_t *ctx = user_ctx;
    BaseType_t high_task_woken = pdFALSE;

    (void) pin;

    ctx->interrupt_count += 1u;
    vTaskNotifyGiveFromISR(ctx->task, &high_task_woken);

    return high_task_woken == pdTRUE;
}

static bool configure_step(blusys_err_t err, const char *step)
{
    if (err == BLUSYS_OK) {
        return true;
    }

    printf("%s error: %s\n", step, blusys_err_string(err));
    return false;
}

void app_main(void)
{
    blusys_gpio_interrupt_ctx_t ctx = {
        .task = xTaskGetCurrentTaskHandle(),
        .interrupt_count = 0u,
    };
    uint32_t printed_count = 0u;

    printf("Blusys gpio interrupt\n");
    printf("target: %s\n", blusys_target_name());
    printf("input_pin: %d\n", BLUSYS_GPIO_INTERRUPT_INPUT_PIN);
    printf("toggle or pulse the input pin to generate interrupts\n");

    if (!configure_step(blusys_gpio_reset(BLUSYS_GPIO_INTERRUPT_INPUT_PIN), "reset input pin") ||
        !configure_step(blusys_gpio_set_input(BLUSYS_GPIO_INTERRUPT_INPUT_PIN), "set input pin")) {
        return;
    }

#if CONFIG_BLUSYS_GPIO_INTERRUPT_PULL_UP
    if (!configure_step(blusys_gpio_set_pull_mode(BLUSYS_GPIO_INTERRUPT_INPUT_PIN, BLUSYS_GPIO_PULL_UP),
                        "set input pull mode")) {
        return;
    }
#endif

    if (!configure_step(blusys_gpio_set_interrupt(BLUSYS_GPIO_INTERRUPT_INPUT_PIN, BLUSYS_GPIO_INTERRUPT_ANY_EDGE),
                        "set interrupt mode") ||
        !configure_step(blusys_gpio_set_callback(BLUSYS_GPIO_INTERRUPT_INPUT_PIN,
                                                 blusys_gpio_interrupt_on_edge,
                                                 &ctx),
                        "set callback")) {
        return;
    }

    while (true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (ctx.interrupt_count != printed_count) {
            printed_count = ctx.interrupt_count;
            printf("interrupt_count=%u\n", (unsigned int) printed_count);
        }
    }
}

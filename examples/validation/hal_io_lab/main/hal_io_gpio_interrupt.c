#include "sdkconfig.h"


#if CONFIG_HAL_IO_LAB_SCENARIO_GPIO_INTERRUPT

#include <stdio.h>

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "blusys/blusys.h"

#define BLUSYS_GPIO_INTERRUPT_INPUT_PIN CONFIG_BLUSYS_GPIO_INTERRUPT_INPUT_PIN
#define BLUSYS_GPIO_INTERRUPT_STAGE1_BIT (1u << 0)
#define BLUSYS_GPIO_INTERRUPT_STAGE2_BIT (1u << 1)

typedef struct {
    TaskHandle_t task;
    volatile uint32_t interrupt_count;
    uint32_t notify_bit;
    const char *label;
} blusys_gpio_interrupt_ctx_t;

static bool blusys_gpio_interrupt_on_edge(int pin, void *user_ctx)
{
    blusys_gpio_interrupt_ctx_t *ctx = user_ctx;
    BaseType_t high_task_woken = pdFALSE;

    (void) pin;

    ctx->interrupt_count += 1u;
    xTaskNotifyFromISR(ctx->task, ctx->notify_bit, eSetBits, &high_task_woken);

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

void run_gpio_interrupt(void)
{
    blusys_gpio_interrupt_ctx_t phase_a = {
        .task = xTaskGetCurrentTaskHandle(),
        .interrupt_count = 0u,
        .notify_bit = BLUSYS_GPIO_INTERRUPT_STAGE1_BIT,
        .label = "phase_a",
    };
    blusys_gpio_interrupt_ctx_t phase_b = {
        .task = xTaskGetCurrentTaskHandle(),
        .interrupt_count = 0u,
        .notify_bit = BLUSYS_GPIO_INTERRUPT_STAGE2_BIT,
        .label = "phase_b",
    };
    uint32_t notified_bits = 0u;

    printf("Blusys gpio interrupt\n");
    printf("target: %s\n", blusys_target_name());
    printf("input_pin: %d\n", BLUSYS_GPIO_INTERRUPT_INPUT_PIN);
    printf("trigger one edge for phase_a, then another edge for phase_b\n");

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
                                                  &phase_a),
                         "set callback")) {
        return;
    }

    while ((notified_bits & BLUSYS_GPIO_INTERRUPT_STAGE1_BIT) == 0u) {
        xTaskNotifyWait(0u, UINT32_MAX, &notified_bits, portMAX_DELAY);
    }

    printf("%s interrupt_count=%u\n", phase_a.label, (unsigned int) phase_a.interrupt_count);

    if (!configure_step(blusys_gpio_set_callback(BLUSYS_GPIO_INTERRUPT_INPUT_PIN,
                                                 blusys_gpio_interrupt_on_edge,
                                                 &phase_b),
                        "replace callback")) {
        return;
    }

    notified_bits = 0u;
    while ((notified_bits & BLUSYS_GPIO_INTERRUPT_STAGE2_BIT) == 0u) {
        xTaskNotifyWait(0u, UINT32_MAX, &notified_bits, portMAX_DELAY);
    }

    printf("%s interrupt_count=%u\n", phase_b.label, (unsigned int) phase_b.interrupt_count);
    printf("callback_swap_result: %s\n",
           ((phase_a.interrupt_count > 0u) && (phase_b.interrupt_count > 0u)) ? "ok" : "incomplete");

    if (!configure_step(blusys_gpio_set_callback(BLUSYS_GPIO_INTERRUPT_INPUT_PIN, NULL, NULL), "clear callback")) {
        return;
    }
}


#else

void run_gpio_interrupt(void) {}

#endif /* CONFIG_HAL_IO_LAB_SCENARIO_GPIO_INTERRUPT */

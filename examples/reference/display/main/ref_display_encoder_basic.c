#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#if CONFIG_DISPLAY_UI_SCENARIO_ENCODER_BASIC

#include "blusys/blusys.h"
#include "blusys/drivers/encoder.h"
#include "driver/gpio.h"

/* Software encoder simulator — drives CLK/DT as INPUT_OUTPUT so the encoder's
 * interrupt stays active while we toggle the pin level from software.
 * Remove this block (and the xTaskCreate call below) once real hardware is verified. */
#define ENCODER_SIM_ENABLED 0

#if ENCODER_SIM_ENABLED
static void encoder_sim_task(void *arg)
{
    (void)arg;
    gpio_num_t clk = CONFIG_BLUSYS_ENCODER_BASIC_CLK_PIN;
    gpio_num_t dt  = CONFIG_BLUSYS_ENCODER_BASIC_DT_PIN;

    /* INPUT_OUTPUT keeps the interrupt active while allowing us to drive the pin */
    gpio_set_direction(clk, GPIO_MODE_INPUT_OUTPUT);
    gpio_set_direction(dt,  GPIO_MODE_INPUT_OUTPUT);
    gpio_set_level(clk, 1);
    gpio_set_level(dt,  1);

    vTaskDelay(pdMS_TO_TICKS(1000));

    while (true) {
        /* One CW detent: CLK↓ → DT↓ → CLK↑ → DT↑  (10 ms per edge) */
        gpio_set_level(clk, 0); vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(dt,  0); vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(clk, 1); vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(dt,  1); vTaskDelay(pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
#endif

static const char *event_name(blusys_encoder_event_t event)
{
    switch (event) {
    case BLUSYS_ENCODER_EVENT_CW:         return "CW";
    case BLUSYS_ENCODER_EVENT_CCW:        return "CCW";
    case BLUSYS_ENCODER_EVENT_PRESS:      return "PRESS";
    case BLUSYS_ENCODER_EVENT_RELEASE:    return "RELEASE";
    case BLUSYS_ENCODER_EVENT_LONG_PRESS: return "LONG_PRESS";
    default:                              return "UNKNOWN";
    }
}

static void on_encoder_event(blusys_encoder_t *enc, blusys_encoder_event_t event,
                              int position, void *ctx)
{
    (void)enc;
    (void)ctx;
    printf("encoder: %-10s  position=%d\n", event_name(event), position);
}

void run_encoder_basic(void)
{
    blusys_encoder_t *enc = NULL;
    blusys_err_t err;

    printf("Blusys encoder basic example\n");
    printf("target: %s\n", blusys_target_name());

    const blusys_encoder_config_t cfg = {
        .clk_pin          = CONFIG_BLUSYS_ENCODER_BASIC_CLK_PIN,
        .dt_pin           = CONFIG_BLUSYS_ENCODER_BASIC_DT_PIN,
        .sw_pin           = CONFIG_BLUSYS_ENCODER_BASIC_SW_PIN,
        .glitch_filter_ns = 1000,
        .debounce_ms      = 50,
        .long_press_ms    = 1000,
    };

    err = blusys_encoder_open(&cfg, &enc);
    if (err != BLUSYS_OK) {
        printf("encoder open failed: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_encoder_set_callback(enc, on_encoder_event, NULL);
    if (err != BLUSYS_OK) {
        printf("set callback failed: %s\n", blusys_err_string(err));
        blusys_encoder_close(enc);
        return;
    }

#if ENCODER_SIM_ENABLED
    printf("*** SIMULATION MODE: software driving CLK/DT pins ***\n");
    xTaskCreate(encoder_sim_task, "enc_sim", 2048, NULL, 5, NULL);
#endif

    printf("waiting for encoder events ...\n");

    while (true) {
        int pos;
        blusys_encoder_get_position(enc, &pos);
        printf("position: %d\n", pos);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

#else /* !CONFIG_DISPLAY_UI_SCENARIO_ENCODER_BASIC */

void run_encoder_basic(void) {}

#endif /* CONFIG_DISPLAY_UI_SCENARIO_ENCODER_BASIC */

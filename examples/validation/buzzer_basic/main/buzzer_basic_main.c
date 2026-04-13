#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "blusys/blusys.h"
#include "blusys/drivers/buzzer.h"

#define BUZZER_PIN  CONFIG_BLUSYS_BUZZER_BASIC_PIN
#define BUZZER_DUTY CONFIG_BLUSYS_BUZZER_BASIC_DUTY_PERMILLE

/* A short ascending jingle: C4 → E4 → G4 with rests between notes. */
static const blusys_buzzer_note_t s_jingle[] = {
    { .freq_hz = 262, .duration_ms = 150 },  /* C4 */
    { .freq_hz =   0, .duration_ms =  50 },  /* rest */
    { .freq_hz = 330, .duration_ms = 150 },  /* E4 */
    { .freq_hz =   0, .duration_ms =  50 },  /* rest */
    { .freq_hz = 392, .duration_ms = 300 },  /* G4 */
};

static void on_buzzer_event(blusys_buzzer_t *buzzer,
                             blusys_buzzer_event_t event,
                             void *user_ctx)
{
    (void)buzzer;
    (void)user_ctx;

    if (event == BLUSYS_BUZZER_EVENT_DONE) {
        printf("buzzer: done\n");
    }
}

void app_main(void)
{
    printf("Blusys buzzer basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d  duty: %d permille\n", BUZZER_PIN, BUZZER_DUTY);

    const blusys_buzzer_config_t cfg = {
        .pin           = BUZZER_PIN,
        .duty_permille = BUZZER_DUTY,
    };

    blusys_buzzer_t *bz = NULL;
    blusys_err_t err = blusys_buzzer_open(&cfg, &bz);
    if (err != BLUSYS_OK) {
        printf("buzzer open failed: %s\n", blusys_err_string(err));
        return;
    }

    blusys_buzzer_set_callback(bz, on_buzzer_event, NULL);

    /* 1. Single 440 Hz tone for 500 ms. */
    printf("playing single tone 440 Hz for 500 ms\n");
    blusys_buzzer_play(bz, 440, 500);
    vTaskDelay(pdMS_TO_TICKS(800));

    /* 2. Jingle sequence with rests. */
    printf("playing jingle sequence\n");
    blusys_buzzer_play_sequence(bz, s_jingle, sizeof(s_jingle) / sizeof(s_jingle[0]));
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* 3. Start a long tone and stop it early. */
    printf("playing 5s tone — stopping after 300 ms\n");
    blusys_buzzer_play(bz, 1000, 5000);
    vTaskDelay(pdMS_TO_TICKS(300));
    blusys_buzzer_stop(bz);
    printf("stopped early (no DONE event expected)\n");
    vTaskDelay(pdMS_TO_TICKS(200));

    blusys_buzzer_close(bz);
    printf("done\n");
}

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "blusys/blusys.h"
#include "blusys/sensor/dht.h"

#define DHT_PIN  CONFIG_BLUSYS_DHT_BASIC_PIN

#ifdef CONFIG_BLUSYS_DHT_BASIC_TYPE_DHT11
#define DHT_TYPE BLUSYS_DHT_TYPE_DHT11
#define DHT_NAME "DHT11"
#else
#define DHT_TYPE BLUSYS_DHT_TYPE_DHT22
#define DHT_NAME "DHT22"
#endif

#define NUM_READINGS 10
#define READ_INTERVAL_MS 3000   /* ≥ 2 s between DHT reads */

void app_main(void)
{
    printf("Blusys DHT basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("sensor: %s  pin: %d\n", DHT_NAME, DHT_PIN);
    printf("dht supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_DHT) ? "yes" : "no");

    const blusys_dht_config_t cfg = {
        .pin  = DHT_PIN,
        .type = DHT_TYPE,
    };

    blusys_dht_t *dht = NULL;
    blusys_err_t err = blusys_dht_open(&cfg, &dht);
    if (err != BLUSYS_OK) {
        printf("dht open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("dht open: ok\n");

    /* Wait for sensor power-on stabilisation (1 s recommended by datasheet) */
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (int i = 1; i <= NUM_READINGS; i++) {
        blusys_dht_reading_t reading;
        err = blusys_dht_read(dht, &reading);
        if (err != BLUSYS_OK) {
            printf("read %d failed: %s\n", i, blusys_err_string(err));
        } else {
            printf("read %d: %.1f C  %.1f %%RH\n",
                   i, reading.temperature_c, reading.humidity_pct);
        }
        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }

    err = blusys_dht_close(dht);
    if (err != BLUSYS_OK) {
        printf("dht close failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("done\n");
}

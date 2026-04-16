#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "blusys/blusys.h"

#if CONFIG_CONNECTIVITY_SCENARIO_MQTT_BASIC

#define MQTT_TOPIC "blusys/test"

static void on_message(const char *topic, const uint8_t *payload, size_t payload_len,
                        void *user_ctx)
{
    printf("Received [%s]: %.*s\n", topic, (int)payload_len, (const char *)payload);
}

void run_mqtt_basic(void)
{
    blusys_err_t err;

    /* Connect to WiFi */
    blusys_wifi_t *wifi;
    blusys_wifi_sta_config_t wifi_cfg = {
        .ssid     = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
    };

    err = blusys_wifi_open(&wifi_cfg, &wifi);
    if (err != BLUSYS_OK) {
        printf("wifi_open failed: %s\n", blusys_err_string(err));
        return;
    }

    printf("Connecting to '%s'...\n", CONFIG_WIFI_SSID);

    err = blusys_wifi_connect(wifi, CONFIG_WIFI_CONNECT_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        printf("wifi_connect failed: %s\n", blusys_err_string(err));
        blusys_wifi_close(wifi);
        return;
    }

    printf("WiFi connected.\n");

    /* Open MQTT client */
    blusys_mqtt_t *mqtt;
    blusys_mqtt_config_t mqtt_cfg = {
        .broker_url    = CONFIG_MQTT_BROKER_URL,
        .timeout_ms    = CONFIG_MQTT_CONNECT_TIMEOUT_MS,
        .message_cb    = on_message,
        .user_ctx      = NULL,
    };

    err = blusys_mqtt_open(&mqtt_cfg, &mqtt);
    if (err != BLUSYS_OK) {
        printf("mqtt_open failed: %s\n", blusys_err_string(err));
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }

    /* Connect to broker */
    printf("Connecting to broker '%s'...\n", CONFIG_MQTT_BROKER_URL);

    err = blusys_mqtt_connect(mqtt);
    if (err != BLUSYS_OK) {
        printf("mqtt_connect failed: %s\n", blusys_err_string(err));
        blusys_mqtt_close(mqtt);
        blusys_wifi_disconnect(wifi);
        blusys_wifi_close(wifi);
        return;
    }

    printf("MQTT connected.\n");

    /* Subscribe to test topic */
    err = blusys_mqtt_subscribe(mqtt, MQTT_TOPIC, BLUSYS_MQTT_QOS_1);
    if (err != BLUSYS_OK) {
        printf("mqtt_subscribe failed: %s\n", blusys_err_string(err));
    } else {
        printf("Subscribed to '%s'.\n", MQTT_TOPIC);
    }

    /* Publish a message — the broker will echo it back via our subscription */
    const char *msg = "hello from blusys";
    err = blusys_mqtt_publish(mqtt, MQTT_TOPIC,
                               (const uint8_t *)msg, strlen(msg),
                               BLUSYS_MQTT_QOS_1, false);
    if (err != BLUSYS_OK) {
        printf("mqtt_publish failed: %s\n", blusys_err_string(err));
    } else {
        printf("Published '%s' to '%s'.\n", msg, MQTT_TOPIC);
    }

    /* Wait for the echo to arrive via the subscription callback */
    vTaskDelay(pdMS_TO_TICKS(5000));

    /* Cleanup */
    blusys_mqtt_disconnect(mqtt);
    blusys_mqtt_close(mqtt);
    blusys_wifi_disconnect(wifi);
    blusys_wifi_close(wifi);

    printf("Done.\n");
}

#else /* !CONFIG_CONNECTIVITY_SCENARIO_MQTT_BASIC */

void run_mqtt_basic(void) {}

#endif /* CONFIG_CONNECTIVITY_SCENARIO_MQTT_BASIC */

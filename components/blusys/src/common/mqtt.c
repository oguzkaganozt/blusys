#include "blusys/mqtt.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "mqtt_client.h"

#include "blusys_esp_err.h"
#include "blusys_lock.h"
#include "blusys_timeout.h"

#define MQTT_CONNECTED_BIT    BIT0
#define MQTT_DISCONNECTED_BIT BIT1
#define MQTT_ERROR_BIT        BIT2

struct blusys_mqtt {
    esp_mqtt_client_handle_t  esp_handle;
    blusys_lock_t             lock;
    blusys_mqtt_message_cb_t  message_cb;
    void                     *user_ctx;
    EventGroupHandle_t        event_group;
    volatile bool             connected;
    int                       timeout_ms;
};

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    blusys_mqtt_t *h = handler_args;
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        h->connected = true;
        xEventGroupClearBits(h->event_group, MQTT_DISCONNECTED_BIT | MQTT_ERROR_BIT);
        xEventGroupSetBits(h->event_group, MQTT_CONNECTED_BIT);
        break;

    case MQTT_EVENT_DISCONNECTED:
        h->connected = false;
        xEventGroupClearBits(h->event_group, MQTT_CONNECTED_BIT);
        xEventGroupSetBits(h->event_group, MQTT_DISCONNECTED_BIT);
        break;

    case MQTT_EVENT_ERROR:
        xEventGroupSetBits(h->event_group, MQTT_ERROR_BIT);
        break;

    case MQTT_EVENT_DATA:
        if ((h->message_cb != NULL) &&
            (event->topic != NULL) && (event->topic_len > 0)) {
            /* topic and data from ESP-IDF are NOT null-terminated */
            char *topic = strndup(event->topic, (size_t)event->topic_len);
            if (topic != NULL) {
                h->message_cb(topic,
                              (const uint8_t *)event->data,
                              (size_t)event->data_len,
                              h->user_ctx);
                free(topic);
            }
        }
        break;

    default:
        break;
    }
}

blusys_err_t blusys_mqtt_open(const blusys_mqtt_config_t *config, blusys_mqtt_t **out_handle)
{
    if ((config == NULL) || (config->broker_url == NULL) || (out_handle == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (!blusys_timeout_ms_is_valid(config->timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_mqtt_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    h->event_group = xEventGroupCreate();
    if (h->event_group == NULL) {
        blusys_lock_deinit(&h->lock);
        free(h);
        return BLUSYS_ERR_NO_MEM;
    }

    h->message_cb  = config->message_cb;
    h->user_ctx    = config->user_ctx;
    h->timeout_ms  = config->timeout_ms;

    esp_mqtt_client_config_t esp_cfg = {
        .broker.address.uri              = config->broker_url,
        .broker.verification.certificate = config->server_cert_pem,
        .credentials.username            = config->username,
        .credentials.authentication.password = config->password,
        .credentials.client_id           = config->client_id,
    };

    h->esp_handle = esp_mqtt_client_init(&esp_cfg);
    if (h->esp_handle == NULL) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_init;
    }

    esp_err_t esp_err = esp_mqtt_client_register_event(h->esp_handle,
                                                        ESP_EVENT_ANY_ID,
                                                        mqtt_event_handler,
                                                        h);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_register;
    }

    *out_handle = h;
    return BLUSYS_OK;

fail_register:
    esp_mqtt_client_destroy(h->esp_handle);
fail_init:
    vEventGroupDelete(h->event_group);
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_mqtt_close(blusys_mqtt_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (handle->connected) {
        esp_mqtt_client_stop(handle->esp_handle);
        handle->connected = false;
    }

    esp_mqtt_client_destroy(handle->esp_handle);
    vEventGroupDelete(handle->event_group);

    blusys_lock_give(&handle->lock);
    blusys_lock_deinit(&handle->lock);
    free(handle);
    return BLUSYS_OK;
}

blusys_err_t blusys_mqtt_connect(blusys_mqtt_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    xEventGroupClearBits(handle->event_group,
                         MQTT_CONNECTED_BIT | MQTT_DISCONNECTED_BIT | MQTT_ERROR_BIT);

    esp_err_t esp_err = esp_mqtt_client_start(handle->esp_handle);
    if (esp_err != ESP_OK) {
        blusys_lock_give(&handle->lock);
        return blusys_translate_esp_err(esp_err);
    }

    TickType_t ticks = blusys_timeout_ms_to_ticks(handle->timeout_ms);
    EventBits_t bits = xEventGroupWaitBits(handle->event_group,
                                            MQTT_CONNECTED_BIT | MQTT_ERROR_BIT | MQTT_DISCONNECTED_BIT,
                                            pdFALSE, pdFALSE, ticks);

    if (bits & MQTT_CONNECTED_BIT) {
        err = BLUSYS_OK;
    } else if (bits & (MQTT_ERROR_BIT | MQTT_DISCONNECTED_BIT)) {
        esp_mqtt_client_stop(handle->esp_handle);
        err = BLUSYS_ERR_IO;
    } else {
        esp_mqtt_client_stop(handle->esp_handle);
        err = BLUSYS_ERR_TIMEOUT;
    }

    blusys_lock_give(&handle->lock);
    return err;
}

blusys_err_t blusys_mqtt_disconnect(blusys_mqtt_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_OK;
    }

    esp_err_t esp_err = esp_mqtt_client_stop(handle->esp_handle);
    handle->connected = false;

    blusys_lock_give(&handle->lock);
    return blusys_translate_esp_err(esp_err);
}

blusys_err_t blusys_mqtt_publish(blusys_mqtt_t *handle,
                                  const char    *topic,
                                  const uint8_t *payload,
                                  size_t         payload_len,
                                  blusys_mqtt_qos_t qos)
{
    if ((handle == NULL) || (topic == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_publish(handle->esp_handle,
                                          topic,
                                          (const char *)payload,
                                          (int)payload_len,
                                          (int)qos,
                                          0);

    blusys_lock_give(&handle->lock);
    return (msg_id >= 0) ? BLUSYS_OK : BLUSYS_ERR_IO;
}

blusys_err_t blusys_mqtt_subscribe(blusys_mqtt_t *handle,
                                    const char    *topic,
                                    blusys_mqtt_qos_t qos)
{
    if ((handle == NULL) || (topic == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_subscribe(handle->esp_handle, topic, (int)qos);

    blusys_lock_give(&handle->lock);
    return (msg_id >= 0) ? BLUSYS_OK : BLUSYS_ERR_IO;
}

blusys_err_t blusys_mqtt_unsubscribe(blusys_mqtt_t *handle, const char *topic)
{
    if ((handle == NULL) || (topic == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    int msg_id = esp_mqtt_client_unsubscribe(handle->esp_handle, topic);

    blusys_lock_give(&handle->lock);
    return (msg_id >= 0) ? BLUSYS_OK : BLUSYS_ERR_IO;
}

bool blusys_mqtt_is_connected(blusys_mqtt_t *handle)
{
    if (handle == NULL) {
        return false;
    }
    return handle->connected;
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_mqtt_open(const blusys_mqtt_config_t *config, blusys_mqtt_t **out_handle)
{
    (void) config;
    (void) out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mqtt_close(blusys_mqtt_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mqtt_connect(blusys_mqtt_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mqtt_disconnect(blusys_mqtt_t *handle)
{
    (void) handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mqtt_publish(blusys_mqtt_t *handle,
                                  const char    *topic,
                                  const uint8_t *payload,
                                  size_t         payload_len,
                                  blusys_mqtt_qos_t qos)
{
    (void) handle;
    (void) topic;
    (void) payload;
    (void) payload_len;
    (void) qos;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mqtt_subscribe(blusys_mqtt_t *handle,
                                    const char    *topic,
                                    blusys_mqtt_qos_t qos)
{
    (void) handle;
    (void) topic;
    (void) qos;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_mqtt_unsubscribe(blusys_mqtt_t *handle, const char *topic)
{
    (void) handle;
    (void) topic;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_mqtt_is_connected(blusys_mqtt_t *handle)
{
    (void) handle;
    return false;
}

#endif /* SOC_WIFI_SUPPORTED */

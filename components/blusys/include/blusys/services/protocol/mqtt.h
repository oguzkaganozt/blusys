/**
 * @file mqtt.h
 * @brief MQTT 3.1.1 client with optional TLS and Last-Will-and-Testament.
 *
 * Callbacks run on the MQTT client task — treat them like an ISR: latch a
 * flag or post to your own queue. See docs/services/mqtt.md.
 */

#ifndef BLUSYS_MQTT_H
#define BLUSYS_MQTT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque MQTT client handle. */
typedef struct blusys_mqtt blusys_mqtt_t;

/** @brief MQTT quality-of-service level. */
typedef enum {
    BLUSYS_MQTT_QOS_0 = 0,   /**< At most once. */
    BLUSYS_MQTT_QOS_1 = 1,   /**< At least once. */
    BLUSYS_MQTT_QOS_2 = 2,   /**< Exactly once. */
} blusys_mqtt_qos_t;

/**
 * @brief Incoming-message callback.
 *
 * @param topic        Null-terminated topic string.
 * @param payload      Raw payload bytes (not null-terminated).
 * @param payload_len  Number of bytes in @p payload.
 * @param user_ctx     User pointer supplied via ::blusys_mqtt_config_t::user_ctx.
 */
typedef void (*blusys_mqtt_message_cb_t)(const char    *topic,
                                         const uint8_t *payload,
                                         size_t         payload_len,
                                         void          *user_ctx);

/** @brief Transport state surfaced to the state callback. */
typedef enum {
    BLUSYS_MQTT_STATE_CONNECTED    = 0,   /**< Connected to the broker. */
    BLUSYS_MQTT_STATE_DISCONNECTED = 1,   /**< Cleanly disconnected. */
    BLUSYS_MQTT_STATE_ERROR        = 2,   /**< Transport error. */
} blusys_mqtt_state_t;

/**
 * @brief Transport-state callback.
 * @param state     New state.
 * @param user_ctx  User pointer supplied via ::blusys_mqtt_config_t::user_ctx.
 */
typedef void (*blusys_mqtt_state_cb_t)(blusys_mqtt_state_t state, void *user_ctx);

/** @brief Configuration for ::blusys_mqtt_open. */
typedef struct {
    const char               *broker_url;       /**< Required, e.g. `"mqtt://broker.example.com:1883"`. */
    const char               *username;         /**< NULL for no authentication. */
    const char               *password;         /**< NULL for no authentication. */
    const char               *client_id;        /**< NULL auto-generates via ESP-IDF. */
    const char               *server_cert_pem;  /**< PEM CA cert for MQTTS; NULL skips verification. */
    int                       timeout_ms;       /**< Connect timeout; `BLUSYS_TIMEOUT_FOREVER` to block. */
    blusys_mqtt_message_cb_t  message_cb;       /**< Optional incoming-message callback. */
    blusys_mqtt_state_cb_t    state_cb;         /**< Optional state callback. */
    void                     *user_ctx;         /**< Opaque pointer forwarded to both callbacks. */

    const char               *will_topic;       /**< Optional LWT topic; NULL disables LWT. */
    const char               *will_payload;     /**< Optional LWT payload; may be NULL or empty. */
    size_t                    will_payload_len; /**< `0` calls `strlen(will_payload)` when non-NULL, else 0. */
    blusys_mqtt_qos_t         will_qos;         /**< LWT QoS; ignored when @ref will_topic is NULL. */
    bool                      will_retain;      /**< LWT retain flag; ignored when @ref will_topic is NULL. */
} blusys_mqtt_config_t;

/**
 * @brief Allocate and initialise an MQTT client. Does not connect.
 * @param config      Configuration.
 * @param out_handle  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_mqtt_open(const blusys_mqtt_config_t *config, blusys_mqtt_t **out_handle);

/**
 * @brief Disconnect (if connected) and free the client.
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p handle is NULL.
 */
blusys_err_t blusys_mqtt_close(blusys_mqtt_t *handle);

/**
 * @brief Connect to the broker.
 *
 * Blocks until CONNACK is received, the timeout expires, or a network
 * error occurs.
 *
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_TIMEOUT`, `BLUSYS_ERR_IO`.
 */
blusys_err_t blusys_mqtt_connect(blusys_mqtt_t *handle);

/**
 * @brief Disconnect from the broker.
 * @param handle  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_mqtt_disconnect(blusys_mqtt_t *handle);

/**
 * @brief Publish a message (fire-and-forget).
 *
 * QoS acknowledgements are handled by the ESP-IDF MQTT stack and are not
 * surfaced here.
 *
 * @param handle       Handle.
 * @param topic        Null-terminated topic.
 * @param payload      Payload bytes.
 * @param payload_len  Bytes in @p payload.
 * @param qos          QoS level.
 * @param retain       When `true`, broker stores this as the latest retained payload.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not connected.
 */
blusys_err_t blusys_mqtt_publish(blusys_mqtt_t *handle,
                                  const char    *topic,
                                  const uint8_t *payload,
                                  size_t         payload_len,
                                  blusys_mqtt_qos_t qos,
                                  bool           retain);

/**
 * @brief Subscribe to a topic filter. Messages are delivered via the message callback.
 * @param handle  Handle.
 * @param topic   Topic filter.
 * @param qos     Requested QoS level.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not connected.
 */
blusys_err_t blusys_mqtt_subscribe(blusys_mqtt_t *handle,
                                    const char    *topic,
                                    blusys_mqtt_qos_t qos);

/**
 * @brief Unsubscribe from a topic filter.
 * @param handle  Handle.
 * @param topic   Topic filter.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_STATE` if not connected.
 */
blusys_err_t blusys_mqtt_unsubscribe(blusys_mqtt_t *handle, const char *topic);

/** @brief Whether the client is currently connected to the broker. */
bool blusys_mqtt_is_connected(blusys_mqtt_t *handle);

#ifdef __cplusplus
}
#endif

#endif

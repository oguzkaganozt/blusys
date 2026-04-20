#ifndef BLUSYS_WIFI_H
#define BLUSYS_WIFI_H

#include <stdbool.h>
#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file wifi.h
 * @brief Station-mode WiFi for connecting the ESP32 family to an existing AP.
 *
 * @c open() sets up the driver and @c connect() performs the association; they
 * are deliberately separate so the stack can be initialized at boot and the
 * connection attempt deferred.
 *
 * Only one WiFi owner may be active at a time — do not combine this service
 * with `blusys_espnow`, `blusys_wifi_mesh`, or `blusys_wifi_prov`.
 */

/** @brief Opaque handle to an open WiFi station session. */
typedef struct blusys_wifi blusys_wifi_t;

/** @brief Lifecycle events delivered through @ref blusys_wifi_event_cb_t. */
typedef enum {
    BLUSYS_WIFI_EVENT_STARTED = 0,      /**< Driver started; not yet connected. */
    BLUSYS_WIFI_EVENT_CONNECTING,        /**< Association in progress. */
    BLUSYS_WIFI_EVENT_CONNECTED,         /**< Associated with the AP (no IP yet). */
    BLUSYS_WIFI_EVENT_GOT_IP,            /**< DHCP succeeded; station has an IP address. */
    BLUSYS_WIFI_EVENT_DISCONNECTED,      /**< Association dropped. */
    BLUSYS_WIFI_EVENT_RECONNECTING,      /**< Auto-reconnect attempt in progress. */
    BLUSYS_WIFI_EVENT_STOPPED,           /**< Driver stopped. */
} blusys_wifi_event_t;

/** @brief Classified reason for the most recent disconnect. */
typedef enum {
    BLUSYS_WIFI_DISCONNECT_REASON_UNKNOWN = 0,       /**< No driver-provided reason available. */
    BLUSYS_WIFI_DISCONNECT_REASON_USER_REQUESTED,    /**< Explicit @ref blusys_wifi_disconnect call. */
    BLUSYS_WIFI_DISCONNECT_REASON_AUTH_FAILED,       /**< Wrong password or unsupported auth mode. */
    BLUSYS_WIFI_DISCONNECT_REASON_NO_AP_FOUND,       /**< Configured SSID is not in range. */
    BLUSYS_WIFI_DISCONNECT_REASON_ASSOC_FAILED,      /**< Association handshake rejected by AP. */
    BLUSYS_WIFI_DISCONNECT_REASON_CONNECTION_LOST,   /**< Link dropped after a previously successful connection. */
} blusys_wifi_disconnect_reason_t;

/** @brief DHCP-assigned network parameters returned by @ref blusys_wifi_get_ip_info. */
typedef struct {
    char ip[16];       /**< Dotted-decimal string, e.g. "192.168.1.100". */
    char netmask[16];  /**< Dotted-decimal subnet mask. */
    char gateway[16];  /**< Dotted-decimal default gateway. */
} blusys_wifi_ip_info_t;

/** @brief Extra data delivered with a WiFi lifecycle event. */
typedef struct {
    blusys_wifi_disconnect_reason_t disconnect_reason;     /**< Classified reason for the last disconnect. */
    int                             raw_disconnect_reason; /**< ESP-IDF @c wifi_err_reason_t value, 0 if unavailable. */
    int                             retry_attempt;         /**< Auto-reconnect attempt counter. */
    blusys_wifi_ip_info_t           ip_info;               /**< Current IP info (valid on GOT_IP). */
} blusys_wifi_event_info_t;

/**
 * @brief Asynchronous WiFi lifecycle callback.
 *
 * Runs on the ESP event task — keep it short and non-blocking. Do not call
 * @ref blusys_wifi_connect, @ref blusys_wifi_disconnect, or @ref blusys_wifi_close
 * from within the callback.
 *
 * @param wifi      Handle on which the event fired.
 * @param event     Event identifier.
 * @param info      Extra data; fields that do not apply to @p event are zeroed.
 * @param user_ctx  The @c user_ctx pointer passed in the config.
 */
typedef void (*blusys_wifi_event_cb_t)(blusys_wifi_t *wifi,
                                        blusys_wifi_event_t event,
                                        const blusys_wifi_event_info_t *info,
                                        void *user_ctx);

/** @brief Configuration passed to @ref blusys_wifi_open. */
typedef struct {
    const char              *ssid;               /**< AP SSID (required). */
    const char              *password;           /**< AP password; NULL or "" for open networks. */
    bool                     auto_reconnect;     /**< Reconnect automatically after an unexpected disconnect. */
    int                      reconnect_delay_ms; /**< Delay before reconnect; ≤0 uses the driver default. */
    int                      max_retries;        /**< 0 disables retries, -1 retries forever. */
    blusys_wifi_event_cb_t   on_event;           /**< Optional lifecycle callback. */
    void                    *user_ctx;           /**< Passed back in on_event. */
} blusys_wifi_sta_config_t;

/**
 * @brief Initialize the WiFi stack in station mode.
 *
 * Internally calls @c nvs_flash_init(), @c esp_netif_init(), and
 * @c esp_event_loop_create_default() — do not call these separately in the
 * same application. Does not associate with the AP; call @ref blusys_wifi_connect
 * when you are ready.
 *
 * @param config    SSID and password (required).
 * @param out_wifi  Output handle.
 * @return BLUSYS_OK on success, BLUSYS_ERR_NO_MEM if allocation fails,
 *         BLUSYS_ERR_INTERNAL on driver init failure.
 */
blusys_err_t blusys_wifi_open(const blusys_wifi_sta_config_t *config, blusys_wifi_t **out_wifi);

/**
 * @brief Stop the WiFi stack and free the handle.
 *
 * @param wifi  Handle returned by @ref blusys_wifi_open.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_close(blusys_wifi_t *wifi);

/**
 * @brief Associate with the configured AP and block until an IP is assigned.
 *
 * @param wifi        Open session handle.
 * @param timeout_ms  Maximum wait in milliseconds; pass BLUSYS_TIMEOUT_FOREVER (-1) to block indefinitely.
 *
 * @return BLUSYS_OK when connected and IP assigned, BLUSYS_ERR_TIMEOUT if @p timeout_ms elapsed,
 *         BLUSYS_ERR_IO on connection refused / auth failure,
 *         BLUSYS_ERR_BUSY if another connect attempt is already in progress.
 */
blusys_err_t blusys_wifi_connect(blusys_wifi_t *wifi, int timeout_ms);

/**
 * @brief Disassociate from the current AP (non-blocking).
 *
 * The disconnected event fires asynchronously.
 *
 * @param wifi  Open session handle.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_disconnect(blusys_wifi_t *wifi);

/**
 * @brief Fill @p out_info with the current IP, netmask, and gateway.
 *
 * @param wifi      Open session handle.
 * @param out_info  Output structure.
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_STATE if not connected.
 */
blusys_err_t blusys_wifi_get_ip_info(blusys_wifi_t *wifi, blusys_wifi_ip_info_t *out_info);

/**
 * @brief Return the classified reason for the most recent disconnect.
 *
 * @param wifi  Open session handle.
 * @return One of @ref blusys_wifi_disconnect_reason_t.
 */
blusys_wifi_disconnect_reason_t blusys_wifi_get_last_disconnect_reason(blusys_wifi_t *wifi);

/**
 * @brief Return the raw ESP-IDF disconnect reason code (or 0 if unavailable).
 *
 * @param wifi  Open session handle.
 * @return ESP-IDF @c wifi_err_reason_t integer, or 0 when no driver reason exists.
 */
int          blusys_wifi_get_last_disconnect_reason_raw(blusys_wifi_t *wifi);

/**
 * @brief True when the station currently has an IP address (non-blocking).
 *
 * @param wifi  Open session handle.
 */
bool         blusys_wifi_is_connected(blusys_wifi_t *wifi);

#ifdef __cplusplus
}
#endif

#endif

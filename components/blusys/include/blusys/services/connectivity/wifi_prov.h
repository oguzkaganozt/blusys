#ifndef BLUSYS_WIFI_PROV_H
#define BLUSYS_WIFI_PROV_H

#include <stdbool.h>
#include <stddef.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle for the WiFi provisioning service.
 */
typedef struct blusys_wifi_prov blusys_wifi_prov_t;

/**
 * @brief Transport used to receive credentials from the provisioning client.
 */
typedef enum {
    BLUSYS_WIFI_PROV_TRANSPORT_BLE,     /*!< BLE transport — requires CONFIG_BT_NIMBLE_ENABLED */
    BLUSYS_WIFI_PROV_TRANSPORT_SOFTAP,  /*!< SoftAP transport — no BLE dependency */
} blusys_wifi_prov_transport_t;

/**
 * @brief Events fired via the on_event callback during provisioning.
 */
typedef enum {
    BLUSYS_WIFI_PROV_EVENT_STARTED,               /*!< Service is advertising / AP is up */
    BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED,  /*!< SSID + password delivered by client */
    BLUSYS_WIFI_PROV_EVENT_SUCCESS,               /*!< Credentials validated — device connected */
    BLUSYS_WIFI_PROV_EVENT_FAILED,                /*!< Credential validation failed */
} blusys_wifi_prov_event_t;

/**
 * @brief Credentials received from the provisioning client.
 *
 * Only valid when the event is BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED.
 */
typedef struct {
    char ssid[33];      /*!< SSID of the target AP */
    char password[65];  /*!< Password; empty string for open networks */
} blusys_wifi_prov_credentials_t;

/**
 * @brief Callback fired on each provisioning event.
 *
 * @param event     The provisioning event.
 * @param creds     Filled for CREDENTIALS_RECEIVED only; NULL for all other events.
 * @param user_ctx  The user_ctx pointer passed to blusys_wifi_prov_config_t.
 */
typedef void (*blusys_wifi_prov_cb_t)(blusys_wifi_prov_event_t event,
                                      const blusys_wifi_prov_credentials_t *creds,
                                      void *user_ctx);

/**
 * @brief Configuration passed to blusys_wifi_prov_open().
 */
typedef struct {
    blusys_wifi_prov_transport_t transport;  /*!< BLE or SoftAP transport */
    const char *service_name;               /*!< BLE device name or SoftAP SSID (required) */
    const char *pop;                        /*!< Proof-of-possession string; NULL disables security */
    const char *service_key;                /*!< SoftAP password; NULL = open AP; ignored for BLE */
    blusys_wifi_prov_cb_t on_event;         /*!< Event callback (required) */
    void *user_ctx;                         /*!< Passed back in every on_event call */
} blusys_wifi_prov_config_t;

/**
 * @brief Initialize the WiFi provisioning service.
 *
 * Sets up the WiFi stack and provisioning manager. Only one instance may be open at a time —
 * a second call returns BLUSYS_ERR_INVALID_STATE until the first is closed.
 *
 * Calls nvs_flash_init(), esp_netif_init(), and esp_event_loop_create_default() internally;
 * do not call them separately in the same application.
 *
 * BLE transport requires CONFIG_BT_NIMBLE_ENABLED in sdkconfig. If not enabled,
 * returns BLUSYS_ERR_NOT_SUPPORTED when transport is BLUSYS_WIFI_PROV_TRANSPORT_BLE.
 *
 * @param config    Service configuration (required).
 * @param out_prov  Output handle.
 *
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_prov_open(const blusys_wifi_prov_config_t *config,
                                   blusys_wifi_prov_t **out_prov);

/**
 * @brief Stop and de-initialize the WiFi provisioning service.
 *
 * Stops the provisioning service if running, deinitializes the manager, and frees the handle.
 *
 * @param prov  Handle returned by blusys_wifi_prov_open().
 *
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_prov_close(blusys_wifi_prov_t *prov);

/**
 * @brief Start advertising the provisioning service.
 *
 * For BLE transport, begins BLE advertising. For SoftAP transport, starts the AP and HTTP server.
 * The on_event callback fires BLUSYS_WIFI_PROV_EVENT_STARTED when the service is ready.
 *
 * @param prov  Handle returned by blusys_wifi_prov_open().
 *
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_STATE if already running.
 */
blusys_err_t blusys_wifi_prov_start(blusys_wifi_prov_t *prov);

/**
 * @brief Stop the provisioning service advertisement.
 *
 * @param prov  Handle returned by blusys_wifi_prov_open().
 *
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_prov_stop(blusys_wifi_prov_t *prov);

/**
 * @brief Write the QR code payload to buf.
 *
 * Produces the JSON string expected by the ESP BLE Provisioning / ESP SoftAP Provisioning
 * mobile apps when scanning a QR code, e.g.:
 *   {"ver":"v1","name":"PROV_ABC","pop":"abcd1234","transport":"ble"}
 *
 * @param prov      Handle returned by blusys_wifi_prov_open().
 * @param buf       Output buffer.
 * @param buf_size  Size of buf in bytes; 256 bytes is sufficient.
 *
 * @return BLUSYS_OK on success, BLUSYS_ERR_INVALID_ARG if buf is NULL or buf_size is 0.
 */
blusys_err_t blusys_wifi_prov_get_qr_payload(blusys_wifi_prov_t *prov,
                                              char *buf, size_t buf_size);

/**
 * @brief Check whether the provisioning manager reports stored WiFi credentials.
 *
 * This helper queries the ESP-IDF provisioning manager state. Call it after
 * @ref blusys_wifi_prov_open or after the application has initialized the WiFi
 * provisioning manager separately. If the manager is not initialized yet, this
 * function returns false.
 *
 * @return true if credentials are stored, false otherwise.
 */
bool blusys_wifi_prov_is_provisioned(void);

/**
 * @brief Erase stored WiFi credentials from NVS.
 *
 * Clears the provisioning manager's stored credentials. After this call,
 * @ref blusys_wifi_prov_is_provisioned returns false once the provisioning
 * manager has been initialized again.
 *
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_prov_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_WIFI_PROV_H */

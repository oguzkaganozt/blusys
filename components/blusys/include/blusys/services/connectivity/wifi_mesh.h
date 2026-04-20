/**
 * @file wifi_mesh.h
 * @brief Self-organizing multi-hop mesh network over the ESP-IDF @c esp_mesh stack.
 *
 * Each node acts as a WiFi station (connects to a parent) and a WiFi soft-AP
 * (accepts children), so data can hop across the mesh without a direct path
 * to the router. Initializes the WiFi driver internally — do not combine with
 * `blusys_wifi`, `blusys_espnow`, or `blusys_wifi_prov`.
 */

#ifndef BLUSYS_WIFI_MESH_H
#define BLUSYS_WIFI_MESH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque handle to an open mesh session. */
typedef struct blusys_wifi_mesh blusys_wifi_mesh_t;

/** @brief Lifecycle events delivered through @ref blusys_wifi_mesh_event_cb_t. */
typedef enum {
    BLUSYS_WIFI_MESH_EVENT_STARTED = 0,          /**< mesh stack started */
    BLUSYS_WIFI_MESH_EVENT_STOPPED,              /**< mesh stack stopped */
    BLUSYS_WIFI_MESH_EVENT_PARENT_CONNECTED,     /**< connected to a parent node */
    BLUSYS_WIFI_MESH_EVENT_PARENT_DISCONNECTED,  /**< lost connection to parent */
    BLUSYS_WIFI_MESH_EVENT_CHILD_CONNECTED,      /**< a child node joined */
    BLUSYS_WIFI_MESH_EVENT_CHILD_DISCONNECTED,   /**< a child node left */
    BLUSYS_WIFI_MESH_EVENT_NO_PARENT_FOUND,      /**< no suitable parent found during scan */
    BLUSYS_WIFI_MESH_EVENT_GOT_IP,               /**< root node received an IP from the router */
} blusys_wifi_mesh_event_t;

/**
 * MAC address of a mesh node.
 */
typedef struct {
    uint8_t addr[6]; /**< 6-byte MAC address */
} blusys_wifi_mesh_addr_t;

/**
 * Extra data delivered with a mesh event. Fields that are not relevant to the
 * event type are zeroed.
 */
typedef struct {
    blusys_wifi_mesh_addr_t peer; /**< peer MAC for PARENT_CONNECTED, PARENT_DISCONNECTED,
                                       CHILD_CONNECTED, and CHILD_DISCONNECTED events */
} blusys_wifi_mesh_event_info_t;

/**
 * Callback invoked for each mesh lifecycle event.
 *
 * Called from the system event task — keep it short and non-blocking.
 *
 * @param mesh     Handle returned by blusys_wifi_mesh_open().
 * @param event    Event identifier.
 * @param info     Extra data for the event; may be NULL for events that carry
 *                 no additional information.
 * @param user_ctx Value passed as @c user_ctx in the config.
 */
typedef void (*blusys_wifi_mesh_event_cb_t)(blusys_wifi_mesh_t *mesh,
                                             blusys_wifi_mesh_event_t event,
                                             const blusys_wifi_mesh_event_info_t *info,
                                             void *user_ctx);

/**
 * Configuration passed to blusys_wifi_mesh_open().
 */
typedef struct {
    uint8_t                       mesh_id[6];       /**< 6-byte mesh network identifier (required) */
    const char                   *password;          /**< mesh AP password (6-64 chars, required) */
    uint8_t                       channel;           /**< WiFi channel; 0 = auto */
    const char                   *router_ssid;       /**< router SSID; root node connects here */
    const char                   *router_password;   /**< router password; NULL for open networks */
    int                           max_layer;         /**< max tree depth; 0 = use default (6) */
    int                           max_connections;   /**< max children per node (1-10); 0 = use default (6) */
    blusys_wifi_mesh_event_cb_t   on_event;          /**< optional lifecycle callback */
    void                         *user_ctx;          /**< passed back in on_event */
} blusys_wifi_mesh_config_t;

/**
 * Initializes the WiFi stack, configures the mesh network, and starts the
 * mesh daemon. Only one handle may be open at a time — a second call returns
 * BLUSYS_ERR_INVALID_STATE until blusys_wifi_mesh_close() is called.
 *
 * Internally calls nvs_flash_init(), esp_netif_init(),
 * esp_event_loop_create_default(), and esp_mesh_init(). Do not call these
 * directly in the same application.
 *
 * @param config     Mesh configuration (mesh_id and password are required).
 * @param out_handle Output handle.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_mesh_open(const blusys_wifi_mesh_config_t *config,
                                    blusys_wifi_mesh_t **out_handle);

/**
 * Stops the mesh stack, shuts down WiFi, and frees the handle. After this
 * call the handle is invalid.
 *
 * Do not call from within an on_event callback.
 *
 * @param handle  Handle returned by blusys_wifi_mesh_open().
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_mesh_close(blusys_wifi_mesh_t *handle);

/**
 * Sends a packet to another node identified by its MAC address.
 *
 * The call enqueues the packet into the mesh TX queue and returns
 * immediately — delivery is asynchronous.
 *
 * Maximum payload size is 1500 bytes (the esp_mesh_send() API limit).
 * Use smaller payloads for reliable throughput.
 *
 * @param handle  Open mesh handle.
 * @param dst     Destination node MAC address.
 * @param data    Payload bytes.
 * @param len     Number of bytes to send.
 * @return BLUSYS_OK if the packet was accepted, error otherwise.
 */
blusys_err_t blusys_wifi_mesh_send(blusys_wifi_mesh_t *handle,
                                    const blusys_wifi_mesh_addr_t *dst,
                                    const void *data, size_t len);

/**
 * Receives a packet addressed to this node.
 *
 * Blocks until a packet arrives or the timeout expires. On success,
 * @p src is filled with the sender's MAC and @p *len is updated to the
 * actual number of bytes written into @p buf.
 *
 * @param handle      Open mesh handle.
 * @param src         Output: sender's MAC address; may be NULL to discard.
 * @param buf         Caller-provided receive buffer.
 * @param[in,out] len On entry: buffer capacity. On exit: bytes received.
 * @param timeout_ms  Max wait in milliseconds; pass -1 to block indefinitely.
 * @return BLUSYS_OK on success, BLUSYS_ERR_TIMEOUT if the timeout expired.
 */
blusys_err_t blusys_wifi_mesh_recv(blusys_wifi_mesh_t *handle,
                                    blusys_wifi_mesh_addr_t *src,
                                    void *buf, size_t *len,
                                    int timeout_ms);

/**
 * Returns true if this node is currently the root of the mesh network.
 *
 * @param handle  Open mesh handle (NULL returns false).
 */
bool blusys_wifi_mesh_is_root(blusys_wifi_mesh_t *handle);

/**
 * Returns the current layer (depth) of this node in the mesh tree.
 * The root is layer 1.
 *
 * @param handle     Open mesh handle.
 * @param out_layer  Output layer number.
 * @return BLUSYS_OK on success.
 */
blusys_err_t blusys_wifi_mesh_get_layer(blusys_wifi_mesh_t *handle, int *out_layer);

#ifdef __cplusplus
}
#endif

#endif

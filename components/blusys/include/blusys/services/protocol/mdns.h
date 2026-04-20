/**
 * @file mdns.h
 * @brief Multicast-DNS hostname, service advertisement, and discovery.
 *
 * Publishes `<hostname>.local` and zero or more `_service._proto` records,
 * and queries the local network for peers advertising a given service type.
 * See docs/services/mdns.md.
 */

#ifndef BLUSYS_MDNS_H
#define BLUSYS_MDNS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque mDNS responder handle. */
typedef struct blusys_mdns blusys_mdns_t;

/** @brief Transport protocol for a service record. */
typedef enum {
    BLUSYS_MDNS_PROTO_TCP = 0,   /**< TCP — `_tcp`. */
    BLUSYS_MDNS_PROTO_UDP,       /**< UDP — `_udp`. */
} blusys_mdns_proto_t;

/** @brief One key/value pair in a service TXT record. */
typedef struct {
    const char *key;    /**< TXT record key. */
    const char *value;  /**< TXT record value. */
} blusys_mdns_txt_item_t;

/** @brief Configuration for ::blusys_mdns_open. */
typedef struct {
    const char *hostname;       /**< e.g. `"my-esp32"` — published as `my-esp32.local`. */
    const char *instance_name;  /**< Friendly name (e.g. `"ESP32 Sensor Node"`). */
} blusys_mdns_config_t;

/** @brief One result row from ::blusys_mdns_query. */
typedef struct {
    char     instance_name[64];   /**< Service instance name. */
    char     hostname[64];        /**< `.local` hostname. */
    char     ip[16];              /**< IPv4 address as a dotted string. */
    uint16_t port;                /**< Service port. */
} blusys_mdns_result_t;

/**
 * @brief Start the mDNS responder and publish the hostname.
 * @param config    Configuration.
 * @param out_mdns  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, `BLUSYS_ERR_NO_MEM`,
 *         `BLUSYS_ERR_NOT_SUPPORTED` on targets without WiFi.
 */
blusys_err_t blusys_mdns_open(const blusys_mdns_config_t *config, blusys_mdns_t **out_mdns);

/**
 * @brief Stop the mDNS responder and free the handle.
 * @param mdns  Handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p mdns is NULL.
 */
blusys_err_t blusys_mdns_close(blusys_mdns_t *mdns);

/**
 * @brief Advertise a service record.
 * @param mdns           Handle.
 * @param instance_name  Optional per-service instance name; NULL inherits the config instance name.
 * @param service_type   Service type without leading underscore, e.g. `"http"`.
 * @param proto          TCP or UDP.
 * @param port           Service port.
 * @param txt            Optional TXT items; may be NULL.
 * @param txt_count      Number of items in @p txt.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, or a translated ESP-IDF error.
 */
blusys_err_t blusys_mdns_add_service(blusys_mdns_t *mdns,
                                      const char *instance_name,
                                      const char *service_type,
                                      blusys_mdns_proto_t proto,
                                      uint16_t port,
                                      const blusys_mdns_txt_item_t *txt,
                                      size_t txt_count);

/**
 * @brief Remove a previously advertised service record.
 * @param mdns          Handle.
 * @param service_type  Service type without leading underscore.
 * @param proto         TCP or UDP.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`.
 */
blusys_err_t blusys_mdns_remove_service(blusys_mdns_t *mdns,
                                         const char *service_type,
                                         blusys_mdns_proto_t proto);

/**
 * @brief Query the local network for peers advertising @p service_type.
 * @param mdns          Handle.
 * @param service_type  Service type without leading underscore.
 * @param proto         TCP or UDP.
 * @param timeout_ms    Query timeout in ms.
 * @param results       Output array (caller-allocated).
 * @param max_results   Capacity of @p results.
 * @param out_count     Output: number of rows written (`<= max_results`).
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG`, or a translated ESP-IDF error.
 */
blusys_err_t blusys_mdns_query(blusys_mdns_t *mdns,
                                const char *service_type,
                                blusys_mdns_proto_t proto,
                                int timeout_ms,
                                blusys_mdns_result_t *results,
                                size_t max_results,
                                size_t *out_count);

#ifdef __cplusplus
}
#endif

#endif

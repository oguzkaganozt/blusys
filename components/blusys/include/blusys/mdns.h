#ifndef BLUSYS_MDNS_H
#define BLUSYS_MDNS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_mdns blusys_mdns_t;

typedef enum {
    BLUSYS_MDNS_PROTO_TCP = 0,
    BLUSYS_MDNS_PROTO_UDP,
} blusys_mdns_proto_t;

typedef struct {
    const char *key;
    const char *value;
} blusys_mdns_txt_item_t;

typedef struct {
    const char *hostname;       /**< e.g. "my-esp32" -> my-esp32.local */
    const char *instance_name;  /**< Friendly name (e.g. "ESP32 Sensor Node") */
} blusys_mdns_config_t;

typedef struct {
    char instance_name[64];
    char hostname[64];
    char ip[16];                /**< IPv4 address string */
    uint16_t port;
} blusys_mdns_result_t;

blusys_err_t blusys_mdns_open(const blusys_mdns_config_t *config, blusys_mdns_t **out_mdns);
blusys_err_t blusys_mdns_close(blusys_mdns_t *mdns);

blusys_err_t blusys_mdns_add_service(blusys_mdns_t *mdns,
                                      const char *instance_name,
                                      const char *service_type,
                                      blusys_mdns_proto_t proto,
                                      uint16_t port,
                                      const blusys_mdns_txt_item_t *txt,
                                      size_t txt_count);

blusys_err_t blusys_mdns_remove_service(blusys_mdns_t *mdns,
                                         const char *service_type,
                                         blusys_mdns_proto_t proto);

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

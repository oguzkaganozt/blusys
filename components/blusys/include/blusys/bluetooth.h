#ifndef BLUSYS_BLUETOOTH_H
#define BLUSYS_BLUETOOTH_H

#include <stdint.h>
#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_bluetooth blusys_bluetooth_t;

typedef struct {
    const char *device_name;  /**< BLE GAP name broadcast in advertising data */
} blusys_bluetooth_config_t;

typedef struct {
    char    name[32];   /**< Device name from advertising payload; empty string if not present */
    uint8_t addr[6];    /**< BLE address bytes, wire order (LSB first) */
    int8_t  rssi;       /**< Received signal strength in dBm */
} blusys_bluetooth_scan_result_t;

typedef void (*blusys_bluetooth_scan_cb_t)(const blusys_bluetooth_scan_result_t *result,
                                            void *user_ctx);

blusys_err_t blusys_bluetooth_open(const blusys_bluetooth_config_t *config,
                                    blusys_bluetooth_t **out_bt);
blusys_err_t blusys_bluetooth_close(blusys_bluetooth_t *bt);

blusys_err_t blusys_bluetooth_advertise_start(blusys_bluetooth_t *bt);
blusys_err_t blusys_bluetooth_advertise_stop(blusys_bluetooth_t *bt);

blusys_err_t blusys_bluetooth_scan_start(blusys_bluetooth_t *bt,
                                          blusys_bluetooth_scan_cb_t cb,
                                          void *user_ctx);
blusys_err_t blusys_bluetooth_scan_stop(blusys_bluetooth_t *bt);

#ifdef __cplusplus
}
#endif

#endif

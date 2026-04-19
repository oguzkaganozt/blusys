#ifndef BLUSYS_BLE_GATT_FWD_H
#define BLUSYS_BLE_GATT_FWD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

struct blusys_ble_gatt;
typedef struct blusys_ble_gatt blusys_ble_gatt_t;

typedef void (*blusys_ble_gatt_conn_cb_t)(uint16_t conn_handle, bool connected,
                                           void *user_ctx);

struct blusys_ble_gatt_chr_def;
typedef struct blusys_ble_gatt_chr_def blusys_ble_gatt_chr_def_t;

struct blusys_ble_gatt_svc_def;
typedef struct blusys_ble_gatt_svc_def blusys_ble_gatt_svc_def_t;

struct blusys_ble_gatt_config;
typedef struct blusys_ble_gatt_config blusys_ble_gatt_config_t;

#ifdef __cplusplus
}
#endif

#endif

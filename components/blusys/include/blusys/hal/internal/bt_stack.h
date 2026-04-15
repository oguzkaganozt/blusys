#ifndef BLUSYS_BT_STACK_H
#define BLUSYS_BT_STACK_H

#include "blusys/hal/error.h"

typedef enum {
    BLUSYS_BT_STACK_OWNER_BLUETOOTH = 0,
    BLUSYS_BT_STACK_OWNER_BLE_GATT,
    BLUSYS_BT_STACK_OWNER_USB_HID_BLE,
    BLUSYS_BT_STACK_OWNER_WIFI_PROV_BLE,
    BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE,
} blusys_bt_stack_owner_t;

/* Returns BLUSYS_ERR_BUSY if another owner already holds the controller. */
blusys_err_t blusys_bt_stack_acquire(blusys_bt_stack_owner_t owner);
void blusys_bt_stack_release(blusys_bt_stack_owner_t owner);

#endif

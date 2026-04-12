#ifndef BLUSYS_BT_STACK_H
#define BLUSYS_BT_STACK_H

#include "blusys/error.h"

typedef enum {
    BLUSYS_BT_STACK_OWNER_BLUETOOTH = 0,
    BLUSYS_BT_STACK_OWNER_BLE_GATT,
    BLUSYS_BT_STACK_OWNER_USB_HID_BLE,
    BLUSYS_BT_STACK_OWNER_WIFI_PROV_BLE,
} blusys_bt_stack_owner_t;

blusys_err_t blusys_bt_stack_acquire(blusys_bt_stack_owner_t owner);
void blusys_bt_stack_release(blusys_bt_stack_owner_t owner);

#endif

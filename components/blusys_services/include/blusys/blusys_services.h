#ifndef BLUSYS_SERVICES_H
#define BLUSYS_SERVICES_H

#ifdef __cplusplus
extern "C" {
#endif

/* output */
#include "blusys/output/display.h"

/* input */
#include "blusys/input/usb_hid.h"

/* connectivity — transports */
#include "blusys/connectivity/ble_gatt.h"
#include "blusys/connectivity/bluetooth.h"
#include "blusys/connectivity/espnow.h"
#include "blusys/connectivity/wifi.h"
#include "blusys/connectivity/wifi_mesh.h"
#include "blusys/connectivity/wifi_prov.h"

/* connectivity — protocols */
#include "blusys/connectivity/protocol/http_client.h"
#include "blusys/connectivity/protocol/http_server.h"
#include "blusys/connectivity/protocol/mqtt.h"
#include "blusys/connectivity/protocol/ws_client.h"
#include "blusys/connectivity/protocol/mdns.h"
#include "blusys/connectivity/protocol/sntp.h"
#include "blusys/connectivity/protocol/local_ctrl.h"

/* storage */
#include "blusys/storage/fatfs.h"
#include "blusys/storage/fs.h"

/* device */
#include "blusys/device/console.h"
#include "blusys/device/ota.h"
#include "blusys/device/power_mgmt.h"

#ifdef __cplusplus
}
#endif

#endif

#ifndef BLUSYS_SERVICES_H
#define BLUSYS_SERVICES_H

#ifdef __cplusplus
extern "C" {
#endif

/* display */
#include "blusys/display/ui.h"

/* input */
#include "blusys/input/usb_hid.h"

/* connectivity */
#include "blusys/connectivity/ble_gatt.h"
#include "blusys/connectivity/bluetooth.h"
#include "blusys/connectivity/espnow.h"
#include "blusys/connectivity/mdns.h"
#include "blusys/connectivity/wifi.h"
#include "blusys/connectivity/wifi_mesh.h"
#include "blusys/connectivity/wifi_prov.h"

/* protocol */
#include "blusys/protocol/http_client.h"
#include "blusys/protocol/http_server.h"
#include "blusys/protocol/mqtt.h"
#include "blusys/protocol/ws_client.h"

/* system */
#include "blusys/system/console.h"
#include "blusys/system/fatfs.h"
#include "blusys/system/fs.h"
#include "blusys/system/local_ctrl.h"
#include "blusys/system/ota.h"
#include "blusys/system/power_mgmt.h"
#include "blusys/system/sntp.h"

#ifdef __cplusplus
}
#endif

#endif

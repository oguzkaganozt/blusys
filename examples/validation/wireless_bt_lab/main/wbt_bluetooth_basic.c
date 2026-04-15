#include "sdkconfig.h"

#if CONFIG_WIRELESS_BT_LAB_SCENARIO_BLUETOOTH_BASIC

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "blusys/services/connectivity/bluetooth.h"

static void scan_cb(const blusys_bluetooth_scan_result_t *result, void *user_ctx)
{
    (void)user_ctx;

    /* BLE addresses are wire-order (LSB first); print MSB first for readability */
    char addr_str[18];
    snprintf(addr_str, sizeof(addr_str),
             "%02x:%02x:%02x:%02x:%02x:%02x",
             result->addr[5], result->addr[4], result->addr[3],
             result->addr[2], result->addr[1], result->addr[0]);

    printf("Found: %-32s  addr=%s  rssi=%d dBm\n",
           result->name[0] ? result->name : "(no name)", addr_str, result->rssi);
}

void run_bluetooth_basic(void)
{
    blusys_bluetooth_t *bt = NULL;
    blusys_bluetooth_config_t cfg = {
        .device_name = CONFIG_BT_DEVICE_NAME,
    };

    blusys_err_t err = blusys_bluetooth_open(&cfg, &bt);
    if (err != BLUSYS_OK) {
        printf("bluetooth_open failed: %d\n", err);
        return;
    }

#if CONFIG_BT_MODE_ADVERTISE
    err = blusys_bluetooth_advertise_start(bt);
    if (err != BLUSYS_OK) {
        printf("advertise_start failed: %d\n", err);
        blusys_bluetooth_close(bt);
        return;
    }
    printf("Advertising as '%s' — visible to nearby BLE scanners. Press reset to stop.\n",
           CONFIG_BT_DEVICE_NAME);
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

#elif CONFIG_BT_MODE_SCAN
    err = blusys_bluetooth_scan_start(bt, scan_cb, NULL);
    if (err != BLUSYS_OK) {
        printf("scan_start failed: %d\n", err);
        blusys_bluetooth_close(bt);
        return;
    }
    printf("Scanning for BLE devices... (30 s)\n");
    vTaskDelay(pdMS_TO_TICKS(30000));

    blusys_bluetooth_scan_stop(bt);
    printf("Scan complete.\n");
    blusys_bluetooth_close(bt);
#endif
}


#else

void run_bluetooth_basic(void) {}

#endif /* CONFIG_WIRELESS_BT_LAB_SCENARIO_BLUETOOTH_BASIC */

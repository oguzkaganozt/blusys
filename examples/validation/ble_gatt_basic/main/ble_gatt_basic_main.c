#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/services/connectivity/ble_gatt.h"

/*
 * Custom GATT service with two characteristics:
 *
 *   data_chr  — F_READ | F_NOTIFY
 *     Exposes a monotonically increasing counter. A connected client can
 *     read it at any time; if it subscribes (enables CCCD), the device
 *     sends a notification every 2 seconds.
 *
 *   cfg_chr   — F_READ | F_WRITE
 *     Accepts arbitrary byte writes from the client (simulating runtime
 *     configuration). Read returns the last value written, or zeros if
 *     nothing has been written yet.
 *
 * UUIDs are 128-bit, stored in little-endian byte order as required by
 * blusys_ble_gatt_chr_def_t.  Generate your own with any UUID v4 tool.
 */

/* Service UUID: 12345678-1234-1234-1234-1234567890ab */
static const uint8_t SVC_UUID[16] = {
    0xab, 0x90, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12
};

/* data_chr UUID: 12345678-1234-1234-1234-1234567890cd */
static const uint8_t DATA_CHR_UUID[16] = {
    0xcd, 0x90, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12
};

/* cfg_chr UUID: 12345678-1234-1234-1234-1234567890ef */
static const uint8_t CFG_CHR_UUID[16] = {
    0xef, 0x90, 0x78, 0x56, 0x34, 0x12,
    0x34, 0x12, 0x34, 0x12, 0x34, 0x12,
    0x78, 0x56, 0x34, 0x12
};

/* Shared application state */
static uint32_t   s_counter      = 0;
static uint16_t   s_conn_handle  = 0xFFFF;   /* 0xFFFF == no client */
static bool       s_connected    = false;
static uint8_t    s_cfg_value[20];
static size_t     s_cfg_len      = 0;
static uint16_t   s_data_val_handle;         /* filled by blusys_ble_gatt_open */

/* ── Characteristic callbacks ─────────────────────────────────── */

static int data_chr_read(uint16_t conn_handle, const uint8_t uuid[16],
                          uint8_t *out_data, size_t max_len,
                          size_t *out_len, void *user_ctx)
{
    (void)conn_handle; (void)uuid; (void)user_ctx;
    uint32_t v = s_counter;
    size_t sz = sizeof(v) < max_len ? sizeof(v) : max_len;
    memcpy(out_data, &v, sz);
    *out_len = sz;
    return 0;
}

static int cfg_chr_read(uint16_t conn_handle, const uint8_t uuid[16],
                         uint8_t *out_data, size_t max_len,
                         size_t *out_len, void *user_ctx)
{
    (void)conn_handle; (void)uuid; (void)user_ctx;
    size_t sz = s_cfg_len < max_len ? s_cfg_len : max_len;
    memcpy(out_data, s_cfg_value, sz);
    *out_len = sz;
    return 0;
}

static int cfg_chr_write(uint16_t conn_handle, const uint8_t uuid[16],
                          const uint8_t *data, size_t len, void *user_ctx)
{
    (void)conn_handle; (void)uuid; (void)user_ctx;
    size_t sz = len < sizeof(s_cfg_value) ? len : sizeof(s_cfg_value);
    memcpy(s_cfg_value, data, sz);
    s_cfg_len = sz;
    printf("[ble_gatt] cfg_chr written: %zu byte(s):", sz);
    for (size_t i = 0; i < sz; i++) {
        printf(" %02x", data[i]);
    }
    printf("\n");
    return 0;
}

/* ── Connection callback ──────────────────────────────────────── */

static void on_conn(uint16_t conn_handle, bool connected, void *user_ctx)
{
    (void)user_ctx;
    if (connected) {
        printf("[ble_gatt] client connected (conn_handle=%u)\n", conn_handle);
        s_conn_handle = conn_handle;
        s_connected   = true;
    } else {
        printf("[ble_gatt] client disconnected (conn_handle=%u)\n", conn_handle);
        s_conn_handle = 0xFFFF;
        s_connected   = false;
    }
}

/* ── Notify task ──────────────────────────────────────────────── */

static void notify_task(void *arg)
{
    blusys_ble_gatt_t *gatt = (blusys_ble_gatt_t *)arg;
    int notify_count = 0;

    while (notify_count < 10) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        s_counter++;

        if (s_connected) {
            blusys_err_t err = blusys_ble_gatt_notify(gatt,
                                                       s_conn_handle,
                                                       s_data_val_handle,
                                                       &s_counter,
                                                       sizeof(s_counter));
            if (err == BLUSYS_OK) {
                printf("[ble_gatt] notified counter=%" PRIu32 "\n", s_counter);
                notify_count++;
            } else {
                printf("[ble_gatt] notify failed (%d)\n", err);
            }
        } else {
            printf("[ble_gatt] counter=%" PRIu32 " (no client connected)\n", s_counter);
        }
    }

    printf("[ble_gatt] 10 notifications sent — closing\n");
    blusys_ble_gatt_close(gatt);
    vTaskDelete(NULL);
}

/* ── Entry point ──────────────────────────────────────────────── */

void app_main(void)
{
    /* Characteristic definitions */
    blusys_ble_gatt_chr_def_t chrs[] = {
        {
            .uuid           = { 0 },  /* filled below */
            .flags          = BLUSYS_BLE_GATT_CHR_F_READ | BLUSYS_BLE_GATT_CHR_F_NOTIFY,
            .read_cb        = data_chr_read,
            .val_handle_out = &s_data_val_handle,
        },
        {
            .uuid    = { 0 },         /* filled below */
            .flags   = BLUSYS_BLE_GATT_CHR_F_READ | BLUSYS_BLE_GATT_CHR_F_WRITE,
            .read_cb = cfg_chr_read,
            .write_cb = cfg_chr_write,
        },
    };
    memcpy(chrs[0].uuid, DATA_CHR_UUID, 16);
    memcpy(chrs[1].uuid, CFG_CHR_UUID,  16);

    /* Service definition */
    blusys_ble_gatt_svc_def_t svcs[] = {
        {
            .uuid            = { 0 },  /* filled below */
            .characteristics = chrs,
            .chr_count       = 2,
        },
    };
    memcpy(svcs[0].uuid, SVC_UUID, 16);

    /* Open the GATT server */
    blusys_ble_gatt_config_t config = {
        .device_name    = "BlusysGATT",
        .services       = svcs,
        .svc_count      = 1,
        .conn_cb        = on_conn,
    };

    blusys_ble_gatt_t *gatt = NULL;
    blusys_err_t err = blusys_ble_gatt_open(&config, &gatt);
    if (err != BLUSYS_OK) {
        printf("[ble_gatt] open failed (%d)\n", err);
        return;
    }
    printf("[ble_gatt] advertising as \"BlusysGATT\" — connect with nRF Connect\n");
    printf("[ble_gatt] data_chr val_handle=%u\n", s_data_val_handle);

    /* Notify task runs independently; closes gatt after 10 cycles */
    xTaskCreate(notify_task, "ble_notify", 4096, gatt, 5, NULL);
}

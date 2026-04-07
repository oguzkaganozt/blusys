#include "blusys/connectivity/bluetooth.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"

#if defined(CONFIG_BT_NIMBLE_ENABLED)

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_nvs_init.h"
#include "blusys/internal/blusys_timeout.h"

#define BT_SYNC_BIT BIT0

struct blusys_bluetooth {
    blusys_lock_t                lock;
    char                         device_name[32];
    EventGroupHandle_t           sync_event;
    blusys_bluetooth_scan_cb_t   scan_cb;
    void                        *scan_user_ctx;
    bool                         advertising;
    bool                         scanning;
};

static blusys_bluetooth_t *s_active_handle;

static void ble_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void on_ble_sync(void)
{
    if (s_active_handle) {
        xEventGroupSetBits(s_active_handle->sync_event, BT_SYNC_BIT);
    }
}

static void on_ble_reset(int reason)
{
    (void)reason;
    if (s_active_handle) {
        xEventGroupClearBits(s_active_handle->sync_event, BT_SYNC_BIT);
    }
}

static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)event;
    (void)arg;
    return 0;
}

static int disc_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;

    if (event->type != BLE_GAP_EVENT_DISC) {
        return 0;
    }

    blusys_bluetooth_t *h = s_active_handle;
    if (!h || !h->scan_cb) {
        return 0;
    }

    blusys_bluetooth_scan_result_t result;
    result.name[0] = '\0';
    memcpy(result.addr, event->disc.addr.val, 6);
    result.rssi = (int8_t)event->disc.rssi;

    struct ble_hs_adv_fields fields;
    if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                 event->disc.length_data) == 0
        && fields.name != NULL && fields.name_len > 0) {
        size_t n = fields.name_len < (sizeof(result.name) - 1)
                 ? fields.name_len : (sizeof(result.name) - 1);
        memcpy(result.name, fields.name, n);
    }

    h->scan_cb(&result, h->scan_user_ctx);
    return 0;
}

blusys_err_t blusys_bluetooth_open(const blusys_bluetooth_config_t *config,
                                    blusys_bluetooth_t **out_bt)
{
    if (config == NULL || config->device_name == NULL || out_bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (s_active_handle != NULL) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_bluetooth_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    h->sync_event = xEventGroupCreate();
    if (h->sync_event == NULL) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_event;
    }

    strncpy(h->device_name, config->device_name, sizeof(h->device_name) - 1);
    s_active_handle = h;

    /* NVS is required by the BT controller for RF calibration data */
    err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        goto fail_hci;
    }

    esp_err_t esp_err = nimble_port_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_hci;
    }

    ble_hs_cfg.sync_cb  = on_ble_sync;
    ble_hs_cfg.reset_cb = on_ble_reset;

    ble_svc_gap_device_name_set(h->device_name);
    nimble_port_freertos_init(ble_host_task);

    EventBits_t bits = xEventGroupWaitBits(h->sync_event, BT_SYNC_BIT,
                                            pdFALSE, pdFALSE,
                                            blusys_timeout_ms_to_ticks(5000));
    if (!(bits & BT_SYNC_BIT)) {
        err = BLUSYS_ERR_TIMEOUT;
        goto fail_nimble;
    }

    *out_bt = h;
    return BLUSYS_OK;

fail_nimble:
    nimble_port_stop();
    nimble_port_deinit();
fail_hci:
    s_active_handle = NULL;
    vEventGroupDelete(h->sync_event);
fail_event:
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;
}

blusys_err_t blusys_bluetooth_close(blusys_bluetooth_t *bt)
{
    if (bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&bt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (bt->advertising) {
        ble_gap_adv_stop();
        bt->advertising = false;
    }

    if (bt->scanning) {
        bt->scan_cb       = NULL;
        bt->scan_user_ctx = NULL;
        ble_gap_disc_cancel();
        bt->scanning      = false;
    }

    blusys_lock_give(&bt->lock);

    nimble_port_stop();
    nimble_port_deinit();

    s_active_handle = NULL;
    vEventGroupDelete(bt->sync_event);
    blusys_lock_deinit(&bt->lock);
    free(bt);
    return BLUSYS_OK;
}

blusys_err_t blusys_bluetooth_advertise_start(blusys_bluetooth_t *bt)
{
    if (bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&bt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (bt->advertising) {
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.flags            = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name             = (uint8_t *)bt->device_name;
    fields.name_len         = (uint8_t)strlen(bt->device_name);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INTERNAL;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                            &adv_params, gap_event_cb, NULL);
    if (rc != 0) {
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INTERNAL;
    }

    bt->advertising = true;
    blusys_lock_give(&bt->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_bluetooth_advertise_stop(blusys_bluetooth_t *bt)
{
    if (bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&bt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!bt->advertising) {
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    ble_gap_adv_stop();
    bt->advertising = false;

    blusys_lock_give(&bt->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_bluetooth_scan_start(blusys_bluetooth_t *bt,
                                          blusys_bluetooth_scan_cb_t cb,
                                          void *user_ctx)
{
    if (bt == NULL || cb == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&bt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (bt->scanning) {
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    bt->scan_cb       = cb;
    bt->scan_user_ctx = user_ctx;

    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));
    disc_params.passive           = 1;
    disc_params.filter_duplicates = 1;

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER,
                           &disc_params, disc_event_cb, NULL);
    if (rc != 0) {
        bt->scan_cb       = NULL;
        bt->scan_user_ctx = NULL;
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INTERNAL;
    }

    bt->scanning = true;
    blusys_lock_give(&bt->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_bluetooth_scan_stop(blusys_bluetooth_t *bt)
{
    if (bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&bt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (!bt->scanning) {
        blusys_lock_give(&bt->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    ble_gap_disc_cancel();
    bt->scanning      = false;
    bt->scan_cb       = NULL;
    bt->scan_user_ctx = NULL;

    blusys_lock_give(&bt->lock);
    return BLUSYS_OK;
}

#else /* !CONFIG_BT_NIMBLE_ENABLED */

blusys_err_t blusys_bluetooth_open(const blusys_bluetooth_config_t *config,
                                    blusys_bluetooth_t **out_bt)
{
    (void)config; (void)out_bt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_bluetooth_close(blusys_bluetooth_t *bt)
{
    (void)bt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_bluetooth_advertise_start(blusys_bluetooth_t *bt)
{
    (void)bt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_bluetooth_advertise_stop(blusys_bluetooth_t *bt)
{
    (void)bt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_bluetooth_scan_start(blusys_bluetooth_t *bt,
                                          blusys_bluetooth_scan_cb_t cb,
                                          void *user_ctx)
{
    (void)bt; (void)cb; (void)user_ctx;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_bluetooth_scan_stop(blusys_bluetooth_t *bt)
{
    (void)bt;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* CONFIG_BT_NIMBLE_ENABLED */

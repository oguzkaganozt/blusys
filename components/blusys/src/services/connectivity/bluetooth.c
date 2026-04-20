#include "blusys/services/connectivity/bluetooth.h"

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

#include "blusys/hal/internal/bt_stack.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/global_lock.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/nvs_init.h"
#include "blusys/hal/internal/timeout.h"

#define BT_SYNC_BIT BIT0
#define BT_IDLE_BIT BIT1

struct blusys_bluetooth {
    blusys_lock_t                lock;
    char                         device_name[32];
    uint8_t                      own_addr_type;
    EventGroupHandle_t           sync_event;
    blusys_bluetooth_scan_cb_t   scan_cb;
    void                        *scan_user_ctx;
    bool                         advertising;
    bool                         scanning;
    bool                         closing;
    uint32_t                     callback_depth;
};

static blusys_bluetooth_t *s_active_handle;

BLUSYS_DEFINE_GLOBAL_LOCK(bt);

static void ble_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void bt_callback_enter_locked(blusys_bluetooth_t *h)
{
    if (h->callback_depth++ == 0) {
        xEventGroupClearBits(h->sync_event, BT_IDLE_BIT);
    }
}

static void bt_callback_exit_locked(blusys_bluetooth_t *h)
{
    if (h->callback_depth == 0) {
        return;
    }
    if (--h->callback_depth == 0) {
        xEventGroupSetBits(h->sync_event, BT_IDLE_BIT);
    }
}

static void on_ble_sync(void)
{
    if (blusys_lock_take(&s_bt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_bluetooth_t *h = s_active_handle;
    if (h == NULL) {
        blusys_lock_give(&s_bt_global_lock);
        return;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_bt_global_lock);
        return;
    }
    blusys_lock_give(&s_bt_global_lock);

    if (!h->closing) {
        xEventGroupSetBits(h->sync_event, BT_SYNC_BIT);
    }
    blusys_lock_give(&h->lock);
}

static void on_ble_reset(int reason)
{
    (void)reason;
    if (blusys_lock_take(&s_bt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_bluetooth_t *h = s_active_handle;
    if (h == NULL) {
        blusys_lock_give(&s_bt_global_lock);
        return;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_bt_global_lock);
        return;
    }
    blusys_lock_give(&s_bt_global_lock);

    if (!h->closing) {
        xEventGroupClearBits(h->sync_event, BT_SYNC_BIT);
    }
    blusys_lock_give(&h->lock);
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

    if (blusys_lock_take(&s_bt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return 0;
    }
    blusys_bluetooth_t *h = s_active_handle;
    if (h == NULL) {
        blusys_lock_give(&s_bt_global_lock);
        return 0;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_bt_global_lock);
        return 0;
    }
    blusys_lock_give(&s_bt_global_lock);

    if (h->closing || !h->scanning || h->scan_cb == NULL) {
        blusys_lock_give(&h->lock);
        return 0;
    }

    bt_callback_enter_locked(h);
    blusys_bluetooth_scan_cb_t scan_cb = h->scan_cb;
    void *scan_user_ctx = h->scan_user_ctx;
    blusys_lock_give(&h->lock);

    blusys_bluetooth_scan_result_t result;
    memset(&result, 0, sizeof(result));
    memcpy(result.addr, event->disc.addr.val, 6);
    result.rssi = (int8_t)event->disc.rssi;

    struct ble_hs_adv_fields fields;
    if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                 event->disc.length_data) == 0
        && fields.name != NULL && fields.name_len > 0) {
        size_t n = fields.name_len < (sizeof(result.name) - 1)
                 ? fields.name_len : (sizeof(result.name) - 1);
        memcpy(result.name, fields.name, n);
        result.name[n] = '\0';
    }

    scan_cb(&result, scan_user_ctx);

    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        bt_callback_exit_locked(h);
        blusys_lock_give(&h->lock);
    }
    return 0;
}

blusys_err_t blusys_bluetooth_open(const blusys_bluetooth_config_t *config,
                                    blusys_bluetooth_t **out_bt)
{
    if (config == NULL || config->device_name == NULL || out_bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (strlen(config->device_name) > 29) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = ensure_global_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_bt_stack_acquire(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_bt_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
        return err;
    }

    if (s_active_handle != NULL) {
        blusys_lock_give(&s_bt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_bluetooth_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_bt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_bt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
        free(h);
        return err;
    }

    h->sync_event = xEventGroupCreate();
    if (h->sync_event == NULL) {
        blusys_lock_deinit(&h->lock);
        blusys_lock_give(&s_bt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
        free(h);
        return BLUSYS_ERR_NO_MEM;
    }
    xEventGroupSetBits(h->sync_event, BT_IDLE_BIT);

    strncpy(h->device_name, config->device_name, sizeof(h->device_name) - 1);

    /* Publish handle before releasing lock so callbacks see it immediately */
    s_active_handle = h;
    blusys_lock_give(&s_bt_global_lock);

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

    int rc = ble_hs_id_infer_auto(0, &h->own_addr_type);
    if (rc != 0) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_nimble;
    }

    *out_bt = h;
    return BLUSYS_OK;

fail_nimble:
    nimble_port_stop();
    nimble_port_deinit();
fail_hci:
    if (blusys_lock_take(&s_bt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        if (s_active_handle == h) {
            if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
                h->closing = true;
                s_active_handle = NULL;
                blusys_lock_give(&h->lock);
            } else {
                s_active_handle = NULL;
            }
        }
        blusys_lock_give(&s_bt_global_lock);
    }
    vEventGroupDelete(h->sync_event);
    blusys_lock_deinit(&h->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
    free(h);
    return err;
}

blusys_err_t blusys_bluetooth_close(blusys_bluetooth_t *bt)
{
    if (bt == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&s_bt_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_active_handle != bt) {
        blusys_lock_give(&s_bt_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    /* Clear immediately so concurrent close() calls fail fast and a new
     * open() cannot start until nimble_port_deinit() completes — see
     * the analogous pattern in usb_host.c. */
    err = blusys_lock_take(&bt->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_bt_global_lock);
        return err;
    }

    bt->closing = true;
    s_active_handle = NULL;
    blusys_lock_give(&s_bt_global_lock);

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

    xEventGroupWaitBits(bt->sync_event, BT_IDLE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    nimble_port_stop();
    nimble_port_deinit();

    vEventGroupDelete(bt->sync_event);
    blusys_lock_deinit(&bt->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLUETOOTH);
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

    rc = ble_gap_adv_start(bt->own_addr_type, NULL, BLE_HS_FOREVER,
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

    int rc = ble_gap_disc(bt->own_addr_type, BLE_HS_FOREVER,
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

#endif  /* CONFIG_BT_NIMBLE_ENABLED */

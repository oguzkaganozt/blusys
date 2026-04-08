#include "blusys/connectivity/ble_gatt.h"

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
#include "host/ble_gatt.h"
#include "os/os_mbuf.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "blusys/internal/blusys_bt_stack.h"
#include "blusys/internal/blusys_esp_err.h"
#include "blusys/internal/blusys_lock.h"
#include "blusys/internal/blusys_nvs_init.h"
#include "blusys/internal/blusys_timeout.h"

#define GATT_SYNC_BIT BIT0
#define GATT_IDLE_BIT BIT1
#define GATT_VALUE_BUF_LEN 512u

struct blusys_ble_gatt {
    blusys_lock_t                    lock;
    char                             device_name[32];
    uint8_t                          own_addr_type;
    EventGroupHandle_t               sync_event;
    /* NimBLE service table — allocated on open, freed on close */
    struct ble_gatt_svc_def         *nimble_svcs;
    ble_uuid128_t                   *uuid_pool;
    /* Maps flat characteristic index → user chr def for access callback bridge */
    const blusys_ble_gatt_chr_def_t **chr_map;
    size_t                           chr_map_len;
    /* Connection event callback */
    blusys_ble_gatt_conn_cb_t        conn_cb;
    void                            *conn_user_ctx;
    bool                             closing;
    bool                             ready;
    uint32_t                         callback_depth;
};

static portMUX_TYPE       s_gatt_init_lock = portMUX_INITIALIZER_UNLOCKED;
static blusys_lock_t      s_gatt_global_lock;
static bool               s_gatt_global_lock_inited;
static blusys_ble_gatt_t *s_gatt_handle;

static blusys_err_t ensure_global_lock(void)
{
    if (s_gatt_global_lock_inited) {
        return BLUSYS_OK;
    }
    blusys_lock_t new_lock;
    blusys_err_t err = blusys_lock_init(&new_lock);
    if (err != BLUSYS_OK) {
        return err;
    }
    portENTER_CRITICAL(&s_gatt_init_lock);
    if (!s_gatt_global_lock_inited) {
        s_gatt_global_lock = new_lock;
        s_gatt_global_lock_inited = true;
        portEXIT_CRITICAL(&s_gatt_init_lock);
    } else {
        portEXIT_CRITICAL(&s_gatt_init_lock);
        blusys_lock_deinit(&new_lock);
    }
    return BLUSYS_OK;
}

/* ── Forward declarations ─────────────────────────────────────── */
static int gap_event_cb(struct ble_gap_event *event, void *arg);

/* ── Advertising helper ───────────────────────────────────────── */
static void gatt_callback_enter_locked(blusys_ble_gatt_t *h)
{
    if (h->callback_depth++ == 0) {
        xEventGroupClearBits(h->sync_event, GATT_IDLE_BIT);
    }
}

static void gatt_callback_exit_locked(blusys_ble_gatt_t *h)
{
    if (h->callback_depth == 0) {
        return;
    }
    if (--h->callback_depth == 0) {
        xEventGroupSetBits(h->sync_event, GATT_IDLE_BIT);
    }
}

static blusys_err_t start_advertising(void)
{
    if (blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return BLUSYS_ERR_INTERNAL;
    }
    blusys_ble_gatt_t *h = s_gatt_handle;
    if (!h) {
        blusys_lock_give(&s_gatt_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    /* Copy what we need, then release the global lock before BLE calls */
    char name_copy[32];
    uint8_t own_addr_type = h->own_addr_type;
    memcpy(name_copy, h->device_name, sizeof(name_copy));
    blusys_lock_give(&s_gatt_global_lock);

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.flags            = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name             = (uint8_t *)name_copy;
    fields.name_len         = (uint8_t)strlen(name_copy);
    fields.name_is_complete = 1;
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        return BLUSYS_ERR_INTERNAL;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, gap_event_cb, NULL);
    return rc == 0 ? BLUSYS_OK : BLUSYS_ERR_INTERNAL;
}

/* ── NimBLE host task and sync/reset callbacks ────────────────── */
static void ble_gatt_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void on_gatt_sync(void)
{
    if (blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_ble_gatt_t *h = s_gatt_handle;
    if (h == NULL) {
        blusys_lock_give(&s_gatt_global_lock);
        return;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        return;
    }
    blusys_lock_give(&s_gatt_global_lock);

    bool restart = false;
    if (!h->closing) {
        xEventGroupSetBits(h->sync_event, GATT_SYNC_BIT);
        restart = h->ready;
    }
    blusys_lock_give(&h->lock);

    if (restart) {
        start_advertising();
    }
}

static void on_gatt_reset(int reason)
{
    (void)reason;
    if (blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_ble_gatt_t *h = s_gatt_handle;
    if (h == NULL) {
        blusys_lock_give(&s_gatt_global_lock);
        return;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        return;
    }
    blusys_lock_give(&s_gatt_global_lock);

    if (!h->closing) {
        xEventGroupClearBits(h->sync_event, GATT_SYNC_BIT);
    }
    blusys_lock_give(&h->lock);
}

/* ── GAP event callback ───────────────────────────────────────── */
static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    if (blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return 0;
    }
    blusys_ble_gatt_t *h = s_gatt_handle;
    if (!h) {
        blusys_lock_give(&s_gatt_global_lock);
        return 0;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        return 0;
    }
    blusys_lock_give(&s_gatt_global_lock);

    if (h->closing) {
        blusys_lock_give(&h->lock);
        return 0;
    }

    blusys_ble_gatt_conn_cb_t conn_cb = h->conn_cb;
    void *conn_user_ctx = h->conn_user_ctx;
    bool invoke_conn_cb = false;
    bool connected = false;
    uint16_t conn_handle = 0;
    bool restart_advertising = false;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (conn_cb != NULL) {
            invoke_conn_cb = true;
            connected = event->connect.status == 0;
            conn_handle = event->connect.conn_handle;
            gatt_callback_enter_locked(h);
        }
        if (event->connect.status != 0) {
            restart_advertising = true;
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        if (conn_cb != NULL) {
            invoke_conn_cb = true;
            connected = false;
            conn_handle = event->disconnect.conn.conn_handle;
            gatt_callback_enter_locked(h);
        }
        restart_advertising = true;
        break;

    default:
        break;
    }

    blusys_lock_give(&h->lock);

    if (invoke_conn_cb) {
        conn_cb(conn_handle, connected, conn_user_ctx);
        if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
            gatt_callback_exit_locked(h);
            blusys_lock_give(&h->lock);
        }
    }

    if (restart_advertising) {
        /* Restart advertising so the next client can connect. */
        start_advertising();
    }

    return 0;
}

/* ── GATT characteristic access bridge ───────────────────────── */
static int internal_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)attr_handle;
    if (blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    blusys_ble_gatt_t *h = s_gatt_handle;
    if (!h) {
        blusys_lock_give(&s_gatt_global_lock);
        return BLE_ATT_ERR_UNLIKELY;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        return BLE_ATT_ERR_UNLIKELY;
    }
    blusys_lock_give(&s_gatt_global_lock);

    if (h->closing) {
        blusys_lock_give(&h->lock);
        return BLE_ATT_ERR_UNLIKELY;
    }

    size_t idx = (size_t)(uintptr_t)arg;
    if (idx >= h->chr_map_len || !h->chr_map[idx]) {
        blusys_lock_give(&h->lock);
        return BLE_ATT_ERR_UNLIKELY;
    }
    const blusys_ble_gatt_chr_def_t *chr = h->chr_map[idx];
    uint8_t uuid[16];
    memcpy(uuid, chr->uuid, sizeof(uuid));
    blusys_ble_gatt_read_cb_t read_cb = chr->read_cb;
    blusys_ble_gatt_write_cb_t write_cb = chr->write_cb;
    void *user_ctx = chr->user_ctx;
    bool invokes_user_cb = false;

    if ((ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR && read_cb != NULL) ||
        (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR && write_cb != NULL)) {
        invokes_user_cb = true;
        gatt_callback_enter_locked(h);
    }
    blusys_lock_give(&h->lock);

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        if (!read_cb) {
            return BLE_ATT_ERR_READ_NOT_PERMITTED;
        }
        uint8_t *buf = malloc(GATT_VALUE_BUF_LEN);
        if (buf == NULL) {
            if (invokes_user_cb && blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
                gatt_callback_exit_locked(h);
                blusys_lock_give(&h->lock);
            }
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        size_t out_len = 0;
        int rc = read_cb(conn_handle, uuid, buf, GATT_VALUE_BUF_LEN,
                         &out_len, user_ctx);
        if (invokes_user_cb && blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
            gatt_callback_exit_locked(h);
            blusys_lock_give(&h->lock);
        }
        if (rc != 0) {
            free(buf);
            return rc;
        }
        if (out_len > GATT_VALUE_BUF_LEN) {
            free(buf);
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        rc = os_mbuf_append(ctxt->om, buf, out_len) == 0
             ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        free(buf);
        return rc;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        if (!write_cb) {
            return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
        }
        size_t in_len = OS_MBUF_PKTLEN(ctxt->om);
        if (in_len > GATT_VALUE_BUF_LEN) {
            if (invokes_user_cb && blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
                gatt_callback_exit_locked(h);
                blusys_lock_give(&h->lock);
            }
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        uint8_t *buf = malloc(in_len == 0 ? 1 : in_len);
        if (buf == NULL) {
            if (invokes_user_cb && blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
                gatt_callback_exit_locked(h);
                blusys_lock_give(&h->lock);
            }
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        uint16_t copied = 0;
        int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, in_len, &copied);
        if (rc != 0) {
            rc = BLE_ATT_ERR_UNLIKELY;
        } else if (copied != in_len) {
            rc = BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (rc == 0) {
            rc = write_cb(conn_handle, uuid, buf, in_len, user_ctx);
        }
        if (invokes_user_cb && blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
            gatt_callback_exit_locked(h);
            blusys_lock_give(&h->lock);
        }
        free(buf);
        return rc;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

/* ── NimBLE table cleanup ─────────────────────────────────────── */
static void free_nimble_tables(blusys_ble_gatt_t *h)
{
    if (h->nimble_svcs) {
        for (size_t i = 0; h->nimble_svcs[i].type != 0; i++) {
            free((void *)h->nimble_svcs[i].characteristics);
        }
        free(h->nimble_svcs);
        h->nimble_svcs = NULL;
    }
    free(h->uuid_pool);
    h->uuid_pool = NULL;
    free(h->chr_map);
    h->chr_map = NULL;
}

/* ── Public API ───────────────────────────────────────────────── */
blusys_err_t blusys_ble_gatt_open(const blusys_ble_gatt_config_t *config,
                                   blusys_ble_gatt_t **out_handle)
{
    if (config == NULL || config->device_name == NULL ||
        config->services == NULL || config->svc_count == 0 ||
        out_handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (strlen(config->device_name) > 29) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < config->svc_count; i++) {
        const blusys_ble_gatt_svc_def_t *svc = &config->services[i];
        if (svc->characteristics == NULL || svc->chr_count == 0) {
            return BLUSYS_ERR_INVALID_ARG;
        }
        for (size_t j = 0; j < svc->chr_count; j++) {
            const blusys_ble_gatt_chr_def_t *chr = &svc->characteristics[j];
            if ((chr->flags & BLUSYS_BLE_GATT_CHR_F_READ) && chr->read_cb == NULL) {
                return BLUSYS_ERR_INVALID_ARG;
            }
            if ((chr->flags & BLUSYS_BLE_GATT_CHR_F_WRITE) && chr->write_cb == NULL) {
                return BLUSYS_ERR_INVALID_ARG;
            }
            if ((chr->flags & BLUSYS_BLE_GATT_CHR_F_NOTIFY) && chr->val_handle_out == NULL) {
                return BLUSYS_ERR_INVALID_ARG;
            }
        }
    }

    blusys_err_t gerr = ensure_global_lock();
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    gerr = blusys_bt_stack_acquire(BLUSYS_BT_STACK_OWNER_BLE_GATT);
    if (gerr != BLUSYS_OK) {
        return gerr;
    }

    gerr = blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (gerr != BLUSYS_OK) {
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
        return gerr;
    }

    if (s_gatt_handle != NULL) {
        blusys_lock_give(&s_gatt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_ble_gatt_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_gatt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
        return BLUSYS_ERR_NO_MEM;
    }

    blusys_err_t err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
        free(h);
        return err;
    }

    h->sync_event = xEventGroupCreate();
    if (h->sync_event == NULL) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_lock;
    }
    xEventGroupSetBits(h->sync_event, GATT_IDLE_BIT);

    strncpy(h->device_name, config->device_name, sizeof(h->device_name) - 1);
    h->conn_cb       = config->conn_cb;
    h->conn_user_ctx = config->conn_user_ctx;

    /* Count total characteristics across all services */
    size_t total_chrs = 0;
    for (size_t i = 0; i < config->svc_count; i++) {
        total_chrs += config->services[i].chr_count;
    }

    /* Allocate NimBLE service table and supporting data */
    h->nimble_svcs = calloc(config->svc_count + 1, sizeof(struct ble_gatt_svc_def));
    h->uuid_pool   = calloc(config->svc_count + total_chrs, sizeof(ble_uuid128_t));
    h->chr_map     = calloc(total_chrs, sizeof(blusys_ble_gatt_chr_def_t *));
    h->chr_map_len = total_chrs;

    if (!h->nimble_svcs || !h->uuid_pool || (total_chrs > 0 && !h->chr_map)) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_tables;
    }

    /* Populate NimBLE service/characteristic tables */
    size_t uuid_idx     = 0;
    size_t chr_flat_idx = 0;

    for (size_t si = 0; si < config->svc_count; si++) {
        const blusys_ble_gatt_svc_def_t *svc = &config->services[si];

        /* Service UUID */
        h->uuid_pool[uuid_idx].u.type = BLE_UUID_TYPE_128;
        memcpy(h->uuid_pool[uuid_idx].value, svc->uuid, 16);
        h->nimble_svcs[si].uuid = &h->uuid_pool[uuid_idx].u;
        uuid_idx++;

        /* Characteristic array (+1 for {0} terminator) */
        struct ble_gatt_chr_def *chrs =
            calloc(svc->chr_count + 1, sizeof(struct ble_gatt_chr_def));
        if (!chrs) {
            err = BLUSYS_ERR_NO_MEM;
            goto fail_tables;
        }
        h->nimble_svcs[si].characteristics = chrs;
        /* Set type last so free_nimble_tables terminator detection is correct */
        h->nimble_svcs[si].type = BLE_GATT_SVC_TYPE_PRIMARY;

        for (size_t ci = 0; ci < svc->chr_count; ci++) {
            const blusys_ble_gatt_chr_def_t *chr = &svc->characteristics[ci];

            /* Characteristic UUID */
            h->uuid_pool[uuid_idx].u.type = BLE_UUID_TYPE_128;
            memcpy(h->uuid_pool[uuid_idx].value, chr->uuid, 16);
            chrs[ci].uuid = &h->uuid_pool[uuid_idx].u;
            uuid_idx++;

            chrs[ci].access_cb = internal_access_cb;
            chrs[ci].arg       = (void *)(uintptr_t)chr_flat_idx;

            uint16_t nimble_flags = 0;
            if (chr->flags & BLUSYS_BLE_GATT_CHR_F_READ)   nimble_flags |= BLE_GATT_CHR_F_READ;
            if (chr->flags & BLUSYS_BLE_GATT_CHR_F_WRITE)  nimble_flags |= BLE_GATT_CHR_F_WRITE;
            if (chr->flags & BLUSYS_BLE_GATT_CHR_F_NOTIFY) nimble_flags |= BLE_GATT_CHR_F_NOTIFY;
            chrs[ci].flags = nimble_flags;

            chrs[ci].val_handle = chr->val_handle_out;

            h->chr_map[chr_flat_idx] = chr;
            chr_flat_idx++;
        }
        /* chrs[svc->chr_count] is already zero-initialized — NimBLE terminator */
    }
    /* nimble_svcs[svc_count] is already zero-initialized — NimBLE terminator */

    /* NVS required by the BT controller for RF calibration data */
    err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        goto fail_tables;
    }

    esp_err_t esp_err = nimble_port_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_tables;
    }

    ble_hs_cfg.sync_cb  = on_gatt_sync;
    ble_hs_cfg.reset_cb = on_gatt_reset;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(h->device_name);

    int rc = ble_gatts_count_cfg(h->nimble_svcs);
    if (rc != 0) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_nimble_init;
    }

    rc = ble_gatts_add_svcs(h->nimble_svcs);
    if (rc != 0) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_nimble_init;
    }

    s_gatt_handle = h;
    blusys_lock_give(&s_gatt_global_lock);

    nimble_port_freertos_init(ble_gatt_host_task);

    EventBits_t bits = xEventGroupWaitBits(h->sync_event, GATT_SYNC_BIT,
                                            pdFALSE, pdFALSE,
                                            blusys_timeout_ms_to_ticks(5000));
    if (!(bits & GATT_SYNC_BIT)) {
        err = BLUSYS_ERR_TIMEOUT;
        goto fail_nimble;
    }

    rc = ble_hs_id_infer_auto(0, &h->own_addr_type);
    if (rc != 0) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_nimble;
    }

    err = start_advertising();
    if (err != BLUSYS_OK) {
        goto fail_nimble;
    }

    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        h->ready = true;
        blusys_lock_give(&h->lock);
    }

    *out_handle = h;
    return BLUSYS_OK;

fail_nimble:
    /* Global lock was already released before nimble_port_freertos_init */
    nimble_port_stop();
    nimble_port_deinit();
    if (blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        if (s_gatt_handle == h) {
            s_gatt_handle = NULL;
        }
        blusys_lock_give(&s_gatt_global_lock);
    }
    free_nimble_tables(h);
    vEventGroupDelete(h->sync_event);
    blusys_lock_deinit(&h->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
    free(h);
    return err;

fail_nimble_init:
    /* Global lock is still held */
    nimble_port_deinit();
fail_tables:
    free_nimble_tables(h);
    vEventGroupDelete(h->sync_event);
fail_lock:
    blusys_lock_give(&s_gatt_global_lock);
    blusys_lock_deinit(&h->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
    free(h);
    return err;
}

blusys_err_t blusys_ble_gatt_close(blusys_ble_gatt_t *handle)
{
    if (handle == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_gatt_handle != handle) {
        blusys_lock_give(&s_gatt_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        return err;
    }

    handle->closing = true;
    s_gatt_handle = NULL;
    blusys_lock_give(&s_gatt_global_lock);

    ble_gap_adv_stop();

    blusys_lock_give(&handle->lock);

    xEventGroupWaitBits(handle->sync_event, GATT_IDLE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    nimble_port_stop();
    nimble_port_deinit();

    free_nimble_tables(handle);
    vEventGroupDelete(handle->sync_event);
    blusys_lock_deinit(&handle->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_GATT);
    free(handle);
    return BLUSYS_OK;
}

blusys_err_t blusys_ble_gatt_notify(blusys_ble_gatt_t *handle,
                                     uint16_t conn_handle,
                                     uint16_t chr_val_handle,
                                     const void *data, size_t len)
{
    if (handle == NULL || data == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&s_gatt_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_gatt_handle != handle) {
        blusys_lock_give(&s_gatt_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_gatt_global_lock);
        return err;
    }
    blusys_lock_give(&s_gatt_global_lock);

    if (handle->closing) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (om == NULL) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_NO_MEM;
    }

    int rc = ble_gatts_notify_custom(conn_handle, chr_val_handle, om);
    blusys_lock_give(&handle->lock);
    return rc == 0 ? BLUSYS_OK : BLUSYS_ERR_INTERNAL;
}

#else /* !CONFIG_BT_NIMBLE_ENABLED */

blusys_err_t blusys_ble_gatt_open(const blusys_ble_gatt_config_t *config,
                                   blusys_ble_gatt_t **out_handle)
{
    (void)config;
    (void)out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ble_gatt_close(blusys_ble_gatt_t *handle)
{
    (void)handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ble_gatt_notify(blusys_ble_gatt_t *handle,
                                     uint16_t conn_handle,
                                     uint16_t chr_val_handle,
                                     const void *data, size_t len)
{
    (void)handle;
    (void)conn_handle;
    (void)chr_val_handle;
    (void)data;
    (void)len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* CONFIG_BT_NIMBLE_ENABLED */

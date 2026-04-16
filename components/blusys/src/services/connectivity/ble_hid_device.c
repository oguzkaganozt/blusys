#include "blusys/services/connectivity/ble_hid_device.h"

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
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "host/ble_store.h"

#include "blusys/hal/internal/bt_stack.h"
#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/global_lock.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/nvs_init.h"
#include "blusys/hal/internal/timeout.h"
#include "blusys/hal/log.h"

#define HID_SYNC_BIT BIT0
#define HID_IDLE_BIT BIT1

static const char *TAG = "blusys_ble_hid";

/* ── HID appearance and identifiers ─────────────────────────────── */
#define HID_APPEARANCE_GENERIC   0x03C1u  /* Generic HID */
#define HID_COUNTRY_CODE_NONE    0x00u
#define HID_INFO_FLAGS           0x01u    /* Remote-wake */
#define HID_VERSION_1_11         0x0111u

/* Consumer-Control report: one 1-byte report, no report-ID prefix on the
 * payload (NimBLE input-report convention). Eight bits, one per usage:
 *   bit 0 Vol+  bit 1 Vol-  bit 2 Mute       bit 3 Play/Pause
 *   bit 4 Next  bit 5 Prev  bit 6 Bright+    bit 7 Bright-
 */
static const uint8_t kReportMap[] = {
    0x05, 0x0C,       /* Usage Page (Consumer)                 */
    0x09, 0x01,       /* Usage (Consumer Control)              */
    0xA1, 0x01,       /* Collection (Application)              */
    0x85, 0x01,       /*   Report ID (1)                       */
    0x15, 0x00,       /*   Logical Minimum (0)                 */
    0x25, 0x01,       /*   Logical Maximum (1)                 */
    0x75, 0x01,       /*   Report Size (1)                     */
    0x95, 0x08,       /*   Report Count (8)                    */
    0x09, 0xE9,       /*   Usage (Volume Increment)   bit 0    */
    0x09, 0xEA,       /*   Usage (Volume Decrement)   bit 1    */
    0x09, 0xE2,       /*   Usage (Mute)               bit 2    */
    0x09, 0xCD,       /*   Usage (Play/Pause)         bit 3    */
    0x09, 0xB5,       /*   Usage (Next Track)         bit 4    */
    0x09, 0xB6,       /*   Usage (Prev Track)         bit 5    */
    0x09, 0x6F,       /*   Usage (Brightness Up)      bit 6    */
    0x09, 0x70,       /*   Usage (Brightness Down)    bit 7    */
    0x81, 0x02,       /*   Input (Data, Variable, Absolute)    */
    0xC0              /* End Collection                        */
};

/* ── Descriptor / characteristic UUIDs (16-bit) ─────────────────── */
static const ble_uuid16_t kUuidHidService     = BLE_UUID16_INIT(0x1812);
static const ble_uuid16_t kUuidHidInfo        = BLE_UUID16_INIT(0x2A4A);
static const ble_uuid16_t kUuidReportMap      = BLE_UUID16_INIT(0x2A4B);
static const ble_uuid16_t kUuidHidCtrlPoint   = BLE_UUID16_INIT(0x2A4C);
static const ble_uuid16_t kUuidReport         = BLE_UUID16_INIT(0x2A4D);
static const ble_uuid16_t kUuidReportRef      = BLE_UUID16_INIT(0x2908);

static const ble_uuid16_t kUuidBatteryService = BLE_UUID16_INIT(0x180F);
static const ble_uuid16_t kUuidBatteryLevel   = BLE_UUID16_INIT(0x2A19);

static const ble_uuid16_t kUuidDeviceInfo     = BLE_UUID16_INIT(0x180A);
static const ble_uuid16_t kUuidPnpId          = BLE_UUID16_INIT(0x2A50);

struct blusys_ble_hid_device {
    blusys_lock_t      lock;
    char               device_name[32];
    uint8_t            own_addr_type;
    EventGroupHandle_t sync_event;

    blusys_ble_hid_device_conn_cb_t conn_cb;
    void              *conn_user_ctx;

    uint16_t           conn_handle;
    bool               connected;
    bool               input_subscribed;

    uint16_t           report_val_handle;
    uint16_t           battery_val_handle;

    uint16_t           vendor_id;
    uint16_t           product_id;
    uint16_t           version;
    uint8_t            battery_level;
    uint8_t            consumer_state;

    bool               closing;
    bool               ready;
    uint32_t           callback_depth;
};

static blusys_ble_hid_device_t *s_hid_handle;

BLUSYS_DEFINE_GLOBAL_LOCK(hid);

static int gap_event_cb(struct ble_gap_event *event, void *arg);

/* ── Callback depth bookkeeping (mirror of ble_gatt.c) ───────────── */
static void hid_callback_enter_locked(blusys_ble_hid_device_t *h)
{
    if (h->callback_depth++ == 0) {
        xEventGroupClearBits(h->sync_event, HID_IDLE_BIT);
    }
}

static void hid_callback_exit_locked(blusys_ble_hid_device_t *h)
{
    if (h->callback_depth == 0) {
        return;
    }
    if (--h->callback_depth == 0) {
        xEventGroupSetBits(h->sync_event, HID_IDLE_BIT);
    }
}

/* ── Advertising helper ─────────────────────────────────────────── */
static blusys_err_t start_advertising(void)
{
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return BLUSYS_ERR_INTERNAL;
    }
    blusys_ble_hid_device_t *h = s_hid_handle;
    if (!h) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    char name_copy[32];
    uint8_t own_addr_type = h->own_addr_type;
    memcpy(name_copy, h->device_name, sizeof(name_copy));
    blusys_lock_give(&s_hid_global_lock);

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.appearance        = HID_APPEARANCE_GENERIC;
    fields.appearance_is_present = 1;
    ble_uuid16_t uuids16[] = { BLE_UUID16_INIT(0x1812) };
    fields.uuids16          = uuids16;
    fields.num_uuids16      = 1;
    fields.uuids16_is_complete = 1;
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        return BLUSYS_ERR_INTERNAL;
    }

    /* Scan response carries the full name so the primary adv packet stays
     * within the 31-byte budget when flags + appearance + 16-bit UUID are
     * already present. */
    struct ble_hs_adv_fields scan_rsp;
    memset(&scan_rsp, 0, sizeof(scan_rsp));
    scan_rsp.name             = (uint8_t *)name_copy;
    scan_rsp.name_len         = (uint8_t)strlen(name_copy);
    scan_rsp.name_is_complete = 1;
    rc = ble_gap_adv_rsp_set_fields(&scan_rsp);
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

/* ── NimBLE host task and sync / reset callbacks ────────────────── */
static void ble_hid_host_task(void *param)
{
    (void)param;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void on_hid_sync(void)
{
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_ble_hid_device_t *h = s_hid_handle;
    if (!h) {
        blusys_lock_give(&s_hid_global_lock);
        return;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return;
    }
    blusys_lock_give(&s_hid_global_lock);

    bool restart = false;
    if (!h->closing) {
        xEventGroupSetBits(h->sync_event, HID_SYNC_BIT);
        restart = h->ready && !h->connected;
    }
    blusys_lock_give(&h->lock);

    if (restart) {
        start_advertising();
    }
}

static void on_hid_reset(int reason)
{
    (void)reason;
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_ble_hid_device_t *h = s_hid_handle;
    if (!h) {
        blusys_lock_give(&s_hid_global_lock);
        return;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return;
    }
    blusys_lock_give(&s_hid_global_lock);

    if (!h->closing) {
        xEventGroupClearBits(h->sync_event, HID_SYNC_BIT);
    }
    blusys_lock_give(&h->lock);
}

/* ── GAP event callback ─────────────────────────────────────────── */
static int gap_event_cb(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return 0;
    }
    blusys_ble_hid_device_t *h = s_hid_handle;
    if (!h) {
        blusys_lock_give(&s_hid_global_lock);
        return 0;
    }
    if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return 0;
    }
    blusys_lock_give(&s_hid_global_lock);

    if (h->closing) {
        blusys_lock_give(&h->lock);
        return 0;
    }

    blusys_ble_hid_device_conn_cb_t conn_cb = h->conn_cb;
    void *conn_user_ctx = h->conn_user_ctx;
    bool invoke_cb = false;
    bool connected = false;
    bool restart_advertising = false;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            h->conn_handle = event->connect.conn_handle;
            h->connected = true;
            connected = true;
            BLUSYS_LOGI(TAG, "GAP connect conn=%u — initiating security",
                        event->connect.conn_handle);
            /* Request encryption so the HID Input Report (READ_ENC |
             * NOTIFY) is actually deliverable. Pairing starts automatically;
             * bonded peers reuse stored keys. */
            int sec_rc = ble_gap_security_initiate(h->conn_handle);
            if (sec_rc != 0) {
                BLUSYS_LOGW(TAG, "security_initiate rc=%d (continuing; host may "
                                  "renegotiate)", sec_rc);
            }
        } else {
            BLUSYS_LOGW(TAG, "GAP connect failed status=%d",
                        event->connect.status);
            restart_advertising = true;
        }
        if (conn_cb != NULL) {
            invoke_cb = true;
            hid_callback_enter_locked(h);
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        BLUSYS_LOGI(TAG, "GAP disconnect conn=%u reason=0x%04x",
                    event->disconnect.conn.conn_handle,
                    event->disconnect.reason);
        h->connected = false;
        h->input_subscribed = false;
        h->conn_handle = 0;
        h->consumer_state = 0;
        restart_advertising = true;
        if (conn_cb != NULL) {
            invoke_cb = true;
            connected = false;
            hid_callback_enter_locked(h);
        }
        break;

    case BLE_GAP_EVENT_ENC_CHANGE:
        BLUSYS_LOGI(TAG, "encryption change conn=%u status=%d (0=ok)",
                    event->enc_change.conn_handle, event->enc_change.status);
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        BLUSYS_LOGI(TAG, "subscribe conn=%u attr=%u cur_notify=%d cur_indicate=%d",
                    event->subscribe.conn_handle, event->subscribe.attr_handle,
                    event->subscribe.cur_notify, event->subscribe.cur_indicate);
        if (event->subscribe.attr_handle == h->report_val_handle) {
            bool was = h->input_subscribed;
            h->input_subscribed = (event->subscribe.cur_notify != 0);
            if (h->input_subscribed && !was) {
                BLUSYS_LOGI(TAG, "input report subscribed — HID ready to deliver reports");
            }
        }
        break;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* Peer reconnected with a different LTK. Clear the old bond and
         * accept repairing so stale keys don't block the HID host. */
        BLUSYS_LOGW(TAG, "repeat pairing — deleting stale bond for conn=%u",
                    event->repeat_pairing.conn_handle);
        {
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0) {
                ble_store_util_delete_peer(&desc.peer_id_addr);
            }
        }
        blusys_lock_give(&h->lock);
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    default:
        break;
    }

    blusys_lock_give(&h->lock);

    if (invoke_cb) {
        conn_cb(connected, conn_user_ctx);
        if (blusys_lock_take(&h->lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
            hid_callback_exit_locked(h);
            blusys_lock_give(&h->lock);
        }
    }

    if (restart_advertising) {
        start_advertising();
    }

    return 0;
}

/* ── Characteristic access callbacks ────────────────────────────── */
static int access_report_map(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    return os_mbuf_append(ctxt->om, kReportMap, sizeof(kReportMap)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int access_hid_info(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    uint8_t info[4] = {
        (uint8_t)(HID_VERSION_1_11 & 0xFF),
        (uint8_t)(HID_VERSION_1_11 >> 8),
        HID_COUNTRY_CODE_NONE,
        HID_INFO_FLAGS,
    };
    return os_mbuf_append(ctxt->om, info, sizeof(info)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int access_hid_control_point(uint16_t conn_handle, uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)ctxt;
    (void)arg;
    /* Suspend/exit-suspend writes — accept and ignore. */
    return 0;
}

static int access_report(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    uint8_t value = 0;
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        if (s_hid_handle) {
            value = s_hid_handle->consumer_state;
        }
        blusys_lock_give(&s_hid_global_lock);
    }
    return os_mbuf_append(ctxt->om, &value, sizeof(value)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int access_report_reference(uint16_t conn_handle, uint16_t attr_handle,
                                    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_DSC) {
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    /* Report ID (1) + Report Type (0x01 = Input) */
    uint8_t ref[2] = { 0x01, 0x01 };
    return os_mbuf_append(ctxt->om, ref, sizeof(ref)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int access_battery_level(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    uint8_t level = 100;
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        if (s_hid_handle) {
            level = s_hid_handle->battery_level;
        }
        blusys_lock_give(&s_hid_global_lock);
    }
    return os_mbuf_append(ctxt->om, &level, sizeof(level)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int access_pnp_id(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_READ_NOT_PERMITTED;
    }
    uint16_t vid = 0x05AC, pid = 0x820A, ver = 0x0100;
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        if (s_hid_handle) {
            if (s_hid_handle->vendor_id)  vid = s_hid_handle->vendor_id;
            if (s_hid_handle->product_id) pid = s_hid_handle->product_id;
            if (s_hid_handle->version)    ver = s_hid_handle->version;
        }
        blusys_lock_give(&s_hid_global_lock);
    }
    uint8_t pnp[7] = {
        0x02,                        /* Vendor ID Source = USB-IF */
        (uint8_t)(vid & 0xFF), (uint8_t)(vid >> 8),
        (uint8_t)(pid & 0xFF), (uint8_t)(pid >> 8),
        (uint8_t)(ver & 0xFF), (uint8_t)(ver >> 8),
    };
    return os_mbuf_append(ctxt->om, pnp, sizeof(pnp)) == 0
               ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

/* ── Service / characteristic tables ────────────────────────────── */
static const struct ble_gatt_dsc_def kReportDescriptors[] = {
    {
        .uuid       = &kUuidReportRef.u,
        .att_flags  = BLE_ATT_F_READ,
        .access_cb  = access_report_reference,
    },
    { 0 }
};

static struct ble_gatt_chr_def kHidChrs[] = {
    {
        .uuid      = &kUuidReportMap.u,
        .access_cb = access_report_map,
        .flags     = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
    },
    {
        .uuid      = &kUuidHidInfo.u,
        .access_cb = access_hid_info,
        .flags     = BLE_GATT_CHR_F_READ,
    },
    {
        .uuid      = &kUuidHidCtrlPoint.u,
        .access_cb = access_hid_control_point,
        .flags     = BLE_GATT_CHR_F_WRITE_NO_RSP,
    },
    {
        .uuid        = &kUuidReport.u,
        .access_cb   = access_report,
        .flags       = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC |
                       BLE_GATT_CHR_F_NOTIFY,
        .descriptors = (struct ble_gatt_dsc_def *)kReportDescriptors,
        /* val_handle filled in at open() */
    },
    { 0 }
};

static struct ble_gatt_chr_def kBatteryChrs[] = {
    {
        .uuid      = &kUuidBatteryLevel.u,
        .access_cb = access_battery_level,
        .flags     = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
        /* val_handle filled in at open() */
    },
    { 0 }
};

static struct ble_gatt_chr_def kDeviceInfoChrs[] = {
    {
        .uuid      = &kUuidPnpId.u,
        .access_cb = access_pnp_id,
        .flags     = BLE_GATT_CHR_F_READ,
    },
    { 0 }
};

static struct ble_gatt_svc_def kServices[] = {
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &kUuidHidService.u,
        .characteristics = kHidChrs,
    },
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &kUuidBatteryService.u,
        .characteristics = kBatteryChrs,
    },
    {
        .type            = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid            = &kUuidDeviceInfo.u,
        .characteristics = kDeviceInfoChrs,
    },
    { 0 }
};

/* ── Public API ─────────────────────────────────────────────────── */
blusys_err_t blusys_ble_hid_device_open(const blusys_ble_hid_device_config_t *config,
                                         blusys_ble_hid_device_t **out_handle)
{
    if (!config || !config->device_name || !out_handle) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (strlen(config->device_name) > 29) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = ensure_global_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_bt_stack_acquire(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
        return err;
    }

    if (s_hid_handle != NULL) {
        blusys_lock_give(&s_hid_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_ble_hid_device_t *h = calloc(1, sizeof(*h));
    if (!h) {
        blusys_lock_give(&s_hid_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
        free(h);
        return err;
    }

    h->sync_event = xEventGroupCreate();
    if (!h->sync_event) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_lock;
    }
    xEventGroupSetBits(h->sync_event, HID_IDLE_BIT);

    strncpy(h->device_name, config->device_name, sizeof(h->device_name) - 1);
    h->conn_cb        = config->conn_cb;
    h->conn_user_ctx  = config->conn_user_ctx;
    h->vendor_id      = config->vendor_id;
    h->product_id     = config->product_id;
    h->version        = config->version;
    h->battery_level  = config->initial_battery_pct > 100
                            ? 100 : config->initial_battery_pct;
    if (h->battery_level == 0 && config->initial_battery_pct == 0) {
        h->battery_level = 100; /* default when caller left field zero */
    }

    /* Wire value-handle outputs into the static characteristic tables. The
     * tables are shared across open/close cycles but that is fine because we
     * enforce a single open at a time via the global lock. */
    kHidChrs[3].val_handle     = &h->report_val_handle;
    kBatteryChrs[0].val_handle = &h->battery_val_handle;

    err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        goto fail_event_group;
    }

    esp_err_t esp_err = nimble_port_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_event_group;
    }

    ble_hs_cfg.sync_cb  = on_hid_sync;
    ble_hs_cfg.reset_cb = on_hid_reset;

    /* Just-Works bonding, persisted by the default NimBLE store. */
    ble_hs_cfg.sm_io_cap     = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding    = 1;
    ble_hs_cfg.sm_mitm       = 0;
    ble_hs_cfg.sm_sc         = 1;
    ble_hs_cfg.sm_our_key_dist   = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(h->device_name);
    ble_svc_gap_device_appearance_set(HID_APPEARANCE_GENERIC);

    int rc = ble_gatts_count_cfg(kServices);
    if (rc != 0) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_nimble_init;
    }
    rc = ble_gatts_add_svcs(kServices);
    if (rc != 0) {
        err = BLUSYS_ERR_INTERNAL;
        goto fail_nimble_init;
    }

    s_hid_handle = h;
    blusys_lock_give(&s_hid_global_lock);

    nimble_port_freertos_init(ble_hid_host_task);

    EventBits_t bits = xEventGroupWaitBits(h->sync_event, HID_SYNC_BIT,
                                            pdFALSE, pdFALSE,
                                            blusys_timeout_ms_to_ticks(5000));
    if (!(bits & HID_SYNC_BIT)) {
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
    nimble_port_stop();
    nimble_port_deinit();
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        if (s_hid_handle == h) {
            s_hid_handle = NULL;
        }
        blusys_lock_give(&s_hid_global_lock);
    }
    vEventGroupDelete(h->sync_event);
    blusys_lock_deinit(&h->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
    free(h);
    return err;

fail_nimble_init:
    nimble_port_deinit();
fail_event_group:
    vEventGroupDelete(h->sync_event);
fail_lock:
    blusys_lock_give(&s_hid_global_lock);
    blusys_lock_deinit(&h->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
    free(h);
    return err;
}

blusys_err_t blusys_ble_hid_device_close(blusys_ble_hid_device_t *handle)
{
    if (!handle) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_hid_handle != handle) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return err;
    }

    handle->closing = true;
    s_hid_handle = NULL;
    blusys_lock_give(&s_hid_global_lock);

    ble_gap_adv_stop();
    if (handle->connected) {
        ble_gap_terminate(handle->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }

    blusys_lock_give(&handle->lock);

    xEventGroupWaitBits(handle->sync_event, HID_IDLE_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    nimble_port_stop();
    nimble_port_deinit();

    vEventGroupDelete(handle->sync_event);
    blusys_lock_deinit(&handle->lock);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_BLE_HID_DEVICE);
    free(handle);
    return BLUSYS_OK;
}

bool blusys_ble_hid_device_is_connected(const blusys_ble_hid_device_t *handle)
{
    if (!handle) {
        return false;
    }
    /* connected is updated under handle->lock; a racy read here is acceptable
     * for status queries (callers that must act atomically should use the
     * conn_cb instead). */
    return handle->connected;
}

bool blusys_ble_hid_device_is_ready(const blusys_ble_hid_device_t *handle)
{
    if (!handle) {
        return false;
    }
    return handle->connected && handle->input_subscribed;
}

static int consumer_usage_to_bit(uint16_t usage)
{
    switch (usage) {
    case BLUSYS_HID_USAGE_VOLUME_UP:       return 0;
    case BLUSYS_HID_USAGE_VOLUME_DOWN:     return 1;
    case BLUSYS_HID_USAGE_MUTE:            return 2;
    case BLUSYS_HID_USAGE_PLAY_PAUSE:      return 3;
    case BLUSYS_HID_USAGE_NEXT_TRACK:      return 4;
    case BLUSYS_HID_USAGE_PREV_TRACK:      return 5;
    case BLUSYS_HID_USAGE_BRIGHTNESS_UP:   return 6;
    case BLUSYS_HID_USAGE_BRIGHTNESS_DOWN: return 7;
    default:                               return -1;
    }
}

blusys_err_t blusys_ble_hid_device_send_consumer(blusys_ble_hid_device_t *handle,
                                                  uint16_t usage, bool pressed)
{
    if (!handle) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    int bit = consumer_usage_to_bit(usage);
    if (bit < 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_hid_handle != handle) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return err;
    }
    blusys_lock_give(&s_hid_global_lock);

    if (handle->closing || !handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    if (pressed) {
        handle->consumer_state |= (uint8_t)(1u << bit);
    } else {
        handle->consumer_state &= (uint8_t) ~(1u << bit);
    }
    uint8_t payload = handle->consumer_state;
    uint16_t conn   = handle->conn_handle;
    uint16_t vh     = handle->report_val_handle;
    blusys_lock_give(&handle->lock);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(&payload, sizeof(payload));
    if (!om) {
        return BLUSYS_ERR_NO_MEM;
    }
    int rc = ble_gatts_notify_custom(conn, vh, om);
    if (rc != 0) {
        BLUSYS_LOGW(TAG, "notify_custom rc=%d (usage=0x%04x pressed=%d handle=%u)",
                    rc, (unsigned)usage, pressed ? 1 : 0, vh);
        return BLUSYS_ERR_INTERNAL;
    }
    return BLUSYS_OK;
}

blusys_err_t blusys_ble_hid_device_send_report(blusys_ble_hid_device_t *handle,
                                                const uint8_t *data, size_t len)
{
    if (!handle || !data || len == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_err_t err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_hid_handle != handle) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return err;
    }
    blusys_lock_give(&s_hid_global_lock);

    if (handle->closing || !handle->connected) {
        blusys_lock_give(&handle->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }
    uint16_t conn = handle->conn_handle;
    uint16_t vh   = handle->report_val_handle;
    blusys_lock_give(&handle->lock);

    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
    if (!om) {
        return BLUSYS_ERR_NO_MEM;
    }
    int rc = ble_gatts_notify_custom(conn, vh, om);
    return rc == 0 ? BLUSYS_OK : BLUSYS_ERR_INTERNAL;
}

blusys_err_t blusys_ble_hid_device_set_battery(blusys_ble_hid_device_t *handle,
                                                uint8_t percent)
{
    if (!handle) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (percent > 100) {
        percent = 100;
    }

    blusys_err_t err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }
    if (s_hid_handle != handle) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }
    err = blusys_lock_take(&handle->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        return err;
    }
    blusys_lock_give(&s_hid_global_lock);

    handle->battery_level = percent;
    bool connected = handle->connected && !handle->closing;
    uint16_t conn  = handle->conn_handle;
    uint16_t vh    = handle->battery_val_handle;
    blusys_lock_give(&handle->lock);

    if (!connected) {
        return BLUSYS_OK;
    }
    struct os_mbuf *om = ble_hs_mbuf_from_flat(&percent, sizeof(percent));
    if (!om) {
        return BLUSYS_ERR_NO_MEM;
    }
    int rc = ble_gatts_notify_custom(conn, vh, om);
    return rc == 0 ? BLUSYS_OK : BLUSYS_ERR_INTERNAL;
}

#else /* !CONFIG_BT_NIMBLE_ENABLED */

blusys_err_t blusys_ble_hid_device_open(const blusys_ble_hid_device_config_t *config,
                                         blusys_ble_hid_device_t **out_handle)
{
    (void)config; (void)out_handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ble_hid_device_close(blusys_ble_hid_device_t *handle)
{
    (void)handle;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_ble_hid_device_is_connected(const blusys_ble_hid_device_t *handle)
{
    (void)handle;
    return false;
}

bool blusys_ble_hid_device_is_ready(const blusys_ble_hid_device_t *handle)
{
    (void)handle;
    return false;
}

blusys_err_t blusys_ble_hid_device_send_consumer(blusys_ble_hid_device_t *handle,
                                                  uint16_t usage, bool pressed)
{
    (void)handle; (void)usage; (void)pressed;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ble_hid_device_send_report(blusys_ble_hid_device_t *handle,
                                                const uint8_t *data, size_t len)
{
    (void)handle; (void)data; (void)len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_ble_hid_device_set_battery(blusys_ble_hid_device_t *handle,
                                                uint8_t percent)
{
    (void)handle; (void)percent;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* CONFIG_BT_NIMBLE_ENABLED */

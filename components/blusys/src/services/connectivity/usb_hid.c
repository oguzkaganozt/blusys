#include "blusys/services/connectivity/usb_hid.h"

#include <stdlib.h>
#include <string.h>

#include "sdkconfig.h"
#include "soc/soc_caps.h"

/* ═══════════════════════════════════════════════════════════════════
 * USB transport path (ESP32-S3 only, requires espressif/usb_host_hid)
 * ═══════════════════════════════════════════════════════════════════ */
#if SOC_USB_OTG_SUPPORTED && defined(BLUSYS_HAS_USB_HOST_HID)
#define HAS_USB_TRANSPORT 1
#else
#define HAS_USB_TRANSPORT 0
#endif

/* ═══════════════════════════════════════════════════════════════════
 * BLE transport path (all targets, requires CONFIG_BT_NIMBLE_ENABLED)
 * ═══════════════════════════════════════════════════════════════════ */
#if defined(CONFIG_BT_NIMBLE_ENABLED)
#define HAS_BLE_TRANSPORT 1
#else
#define HAS_BLE_TRANSPORT 0
#endif

#if HAS_USB_TRANSPORT
#include "usb/hid_host.h"
#include "blusys/hal/usb_host.h"
#endif

#if HAS_BLE_TRANSPORT
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

#include "blusys/hal/internal/bt_stack.h"
#include "blusys/hal/internal/nvs_init.h"
#endif

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/global_lock.h"
#include "blusys/hal/internal/lock.h"

/* ─── HID boot report sizes ────────────────────────────────────── */
#define HID_KEYBOARD_BOOT_REPORT_LEN  8
#define HID_MOUSE_BOOT_REPORT_LEN     3

/* ─── BLE HID UUIDs ─────────────────────────────────────────────── */
#if HAS_BLE_TRANSPORT
#define BLE_HID_SVC_UUID16           0x1812
#define BLE_HID_REPORT_CHR_UUID16    0x2A4D
#define BLE_HID_BOOT_KB_UUID16       0x2A22
#define BLE_HID_BOOT_MOUSE_UUID16    0x2A33
#define BLE_CCCD_UUID16              0x2902
#define BLE_HID_SYNC_BIT             BIT0
#define BLE_HID_MAX_REPORT_CHRS      8

struct ble_hid_ctx {
    uint16_t conn_handle;
    uint16_t hid_svc_start;
    uint16_t hid_svc_end;
    uint16_t report_chr_handles[BLE_HID_MAX_REPORT_CHRS];
    uint16_t report_chr_end_handles[BLE_HID_MAX_REPORT_CHRS];
    uint16_t cccd_handles[BLE_HID_MAX_REPORT_CHRS];
    int      num_report_chrs;
    int      discover_idx;
    int      subscribe_idx;
    int      last_report_idx;
};
#endif /* HAS_BLE_TRANSPORT */

/* ─── Handle struct ─────────────────────────────────────────────── */
struct blusys_usb_hid {
    blusys_lock_t              lock;
    blusys_usb_hid_callback_t  callback;
    void                      *user_ctx;
    blusys_usb_hid_transport_t transport;
    volatile bool              stopping;

#if HAS_USB_TRANSPORT
    blusys_usb_host_t         *usb_host;
#endif

#if HAS_BLE_TRANSPORT
    char                       ble_device_name[64];
    uint8_t                    ble_own_addr_type;
    EventGroupHandle_t         ble_sync_event;
    struct ble_hid_ctx         ble_ctx;
    bool                       ble_connected;
#endif
};

/* ─── Singleton ─────────────────────────────────────────────────── */
static blusys_usb_hid_t *s_usb_hid_handle;

BLUSYS_DEFINE_GLOBAL_LOCK(hid);

/* ─── Report parsing helper ─────────────────────────────────────── */
static void dispatch_raw_report(blusys_usb_hid_t *h,
                                const uint8_t *data, size_t len)
{
    if (h->callback == NULL || len == 0) {
        return;
    }

    blusys_usb_hid_event_data_t ev;
    memset(&ev, 0, sizeof(ev));

    if (len >= HID_KEYBOARD_BOOT_REPORT_LEN) {
        ev.event                = BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT;
        ev.data.keyboard.modifier = data[0];
        memcpy(ev.data.keyboard.keycodes, &data[2], 6);
    } else if (len >= HID_MOUSE_BOOT_REPORT_LEN) {
        ev.event             = BLUSYS_USB_HID_EVENT_MOUSE_REPORT;
        ev.data.mouse.buttons = data[0];
        ev.data.mouse.x       = (int8_t)data[1];
        ev.data.mouse.y       = (int8_t)data[2];
        if (len >= 4) {
            ev.data.mouse.wheel = (int8_t)data[3];
        }
    } else {
        /* data is valid for the duration of this synchronous callback */
        ev.event          = BLUSYS_USB_HID_EVENT_RAW_REPORT;
        ev.data.raw.data  = data;
        ev.data.raw.len   = len;
    }

    h->callback(h, &ev, h->user_ctx);
}

/* ═══════════════════════════════════════════════════════════════════
 * USB transport implementation
 * ═══════════════════════════════════════════════════════════════════ */
#if HAS_USB_TRANSPORT

static void usb_hid_host_device_cb(hid_host_device_handle_t hid_device_handle,
                                    const hid_host_interface_event_t event,
                                    void *arg)
{
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;

    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
        uint8_t  data[64];
        size_t   len = 0;
        hid_host_device_get_raw_input_report_data(hid_device_handle,
                                                  data, sizeof(data), &len);
        dispatch_raw_report(h, data, len);
        break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        if (h->callback != NULL) {
            blusys_usb_hid_event_data_t ev = { .event = BLUSYS_USB_HID_EVENT_DISCONNECTED };
            h->callback(h, &ev, h->user_ctx);
        }
        hid_host_device_close(hid_device_handle);
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        break;
    default:
        break;
    }
}

static void usb_hid_host_event_cb(hid_host_device_handle_t hid_device_handle,
                                   const hid_host_driver_event_t event,
                                   void *arg)
{
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;

    if (event == HID_HOST_DRIVER_EVENT_CONNECTED) {
        hid_host_device_config_t dev_cfg = {
            .callback     = usb_hid_host_device_cb,
            .callback_arg = h,
        };
        esp_err_t ret = hid_host_device_open(hid_device_handle, &dev_cfg);
        if (ret == ESP_OK && h->callback != NULL) {
            blusys_usb_hid_event_data_t ev = { .event = BLUSYS_USB_HID_EVENT_CONNECTED };
            h->callback(h, &ev, h->user_ctx);
        }
    }
}

static void usb_host_device_cb(blusys_usb_host_t *host,
                                blusys_usb_host_event_t event,
                                const blusys_usb_host_device_info_t *device,
                                void *user_ctx)
{
    /* The HID class driver handles device-level events itself via its own client.
     * This callback is for generic connect/disconnect logging only. */
    (void)host;
    (void)event;
    (void)device;
    (void)user_ctx;
}

static blusys_err_t usb_transport_open(blusys_usb_hid_t *h)
{
    blusys_err_t err;
    esp_err_t    esp_err;

    blusys_usb_host_config_t host_cfg = {
        .event_cb       = usb_host_device_cb,
        .event_user_ctx = h,
    };
    err = blusys_usb_host_open(&host_cfg, &h->usb_host);
    if (err != BLUSYS_OK) {
        return err;
    }

    hid_host_driver_config_t hid_cfg = {
        .create_background_task = true,
        .task_priority          = 5,
        .stack_size             = 4096,
        .core_id                = 0,
        .callback               = usb_hid_host_event_cb,
        .callback_arg           = h,
    };
    esp_err = hid_host_install(&hid_cfg);
    if (esp_err != ESP_OK) {
        blusys_usb_host_close(h->usb_host);
        h->usb_host = NULL;
        return blusys_translate_esp_err(esp_err);
    }

    return BLUSYS_OK;
}

static blusys_err_t usb_transport_close(blusys_usb_hid_t *h)
{
    hid_host_uninstall();
    blusys_usb_host_close(h->usb_host);
    h->usb_host = NULL;
    return BLUSYS_OK;
}

#endif /* HAS_USB_TRANSPORT */

/* ═══════════════════════════════════════════════════════════════════
 * BLE transport implementation (NimBLE HOGP central)
 * ═══════════════════════════════════════════════════════════════════ */
#if HAS_BLE_TRANSPORT

static void ble_hid_host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* Forward declarations */
static int  ble_hid_start_scan(blusys_usb_hid_t *h);
static void ble_hid_discover_next_cccd(blusys_usb_hid_t *h);
static void ble_hid_subscribe_next(blusys_usb_hid_t *h);
static int  ble_hid_gap_event(struct ble_gap_event *event, void *arg);

static void on_ble_hid_sync(void)
{
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_usb_hid_t *h = s_usb_hid_handle;
    if (h != NULL) {
        xEventGroupSetBits(h->ble_sync_event, BLE_HID_SYNC_BIT);
    }
    blusys_lock_give(&s_hid_global_lock);
}

static void on_ble_hid_reset(int reason)
{
    (void)reason;
    if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) != BLUSYS_OK) {
        return;
    }
    blusys_usb_hid_t *h = s_usb_hid_handle;
    if (h != NULL) {
        xEventGroupClearBits(h->ble_sync_event, BLE_HID_SYNC_BIT);
    }
    blusys_lock_give(&s_hid_global_lock);
}

static int ble_hid_start_scan(blusys_usb_hid_t *h)
{
    if (h->stopping) {
        return 0;
    }

    struct ble_gap_disc_params disc_params = {
        .itvl          = BLE_GAP_SCAN_FAST_INTERVAL_MIN,
        .window        = BLE_GAP_SCAN_FAST_WINDOW,
        .filter_policy = BLE_HCI_SCAN_FILT_NO_WL,
        .limited       = 0,
        .passive       = 0,
    };
    return ble_gap_disc(h->ble_own_addr_type, BLE_HS_FOREVER, &disc_params,
                        ble_hid_gap_event, h);
}

/* Check advertisement data for HID service UUID (0x1812) */
static bool ble_hid_adv_has_hid_svc(const uint8_t *adv_data, uint8_t adv_len)
{
    for (int i = 0; i + 2 < adv_len; ) {
        uint8_t len  = adv_data[i];
        uint8_t type = adv_data[i + 1];
        if (len < 2) {
            break;
        }
        /* Complete (0x03) or incomplete (0x02) list of 16-bit UUIDs */
        if (type == 0x02 || type == 0x03) {
            int end = i + 1 + len; /* exclusive end of this TLV element */
            for (int j = i + 2; j + 1 < end && j + 1 < adv_len; j += 2) {
                uint16_t uuid = (uint16_t)(adv_data[j] | (adv_data[j + 1] << 8));
                if (uuid == BLE_HID_SVC_UUID16) {
                    return true;
                }
            }
        }
        i += 1 + len;
    }
    return false;
}

static int ble_hid_chr_disc_cb(uint16_t conn_handle,
                                const struct ble_gatt_error *error,
                                const struct ble_gatt_chr *chr,
                                void *arg)
{
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;

    if (error->status == BLE_HS_EDONE) {
        /* Discover the CCCD for each report characteristic before subscribing. */
        h->ble_ctx.discover_idx = 0;
        h->ble_ctx.subscribe_idx = 0;
        ble_hid_discover_next_cccd(h);
        return 0;
    }
    if (error->status != 0) {
        return 0;
    }

    if (h->ble_ctx.last_report_idx >= 0) {
        h->ble_ctx.report_chr_end_handles[h->ble_ctx.last_report_idx] =
            chr->def_handle - 1;
        h->ble_ctx.last_report_idx = -1;
    }

    uint16_t uuid16 = ble_uuid_u16(&chr->uuid.u);
    bool is_report = (uuid16 == BLE_HID_REPORT_CHR_UUID16 ||
                      uuid16 == BLE_HID_BOOT_KB_UUID16 ||
                      uuid16 == BLE_HID_BOOT_MOUSE_UUID16);
    if (is_report && h->ble_ctx.num_report_chrs < BLE_HID_MAX_REPORT_CHRS) {
        int idx = h->ble_ctx.num_report_chrs++;
        h->ble_ctx.report_chr_handles[idx] = chr->val_handle;
        h->ble_ctx.report_chr_end_handles[idx] = h->ble_ctx.hid_svc_end;
        h->ble_ctx.last_report_idx = idx;
    }
    (void)conn_handle;
    return 0;
}

static int ble_hid_dsc_disc_cb(uint16_t conn_handle,
                                const struct ble_gatt_error *error,
                                uint16_t chr_val_handle,
                                const struct ble_gatt_dsc *dsc,
                                void *arg)
{
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;
    int idx = h->ble_ctx.discover_idx;

    if (error->status == BLE_HS_EDONE) {
        h->ble_ctx.discover_idx++;
        ble_hid_discover_next_cccd(h);
        return 0;
    }
    if (error->status != 0) {
        h->ble_ctx.discover_idx++;
        ble_hid_discover_next_cccd(h);
        return 0;
    }

    if (idx >= 0 && idx < h->ble_ctx.num_report_chrs &&
        ble_uuid_u16(&dsc->uuid.u) == BLE_CCCD_UUID16) {
        h->ble_ctx.cccd_handles[idx] = dsc->handle;
    }

    (void)conn_handle;
    (void)chr_val_handle;
    return 0;
}

static void ble_hid_discover_next_cccd(blusys_usb_hid_t *h)
{
    while (h->ble_ctx.discover_idx < h->ble_ctx.num_report_chrs) {
        int idx = h->ble_ctx.discover_idx;
        uint16_t start_handle = h->ble_ctx.report_chr_handles[idx];
        uint16_t end_handle = h->ble_ctx.report_chr_end_handles[idx];

        if (end_handle <= start_handle) {
            h->ble_ctx.discover_idx++;
            continue;
        }

        int rc = ble_gattc_disc_all_dscs(h->ble_ctx.conn_handle,
                                          start_handle,
                                          end_handle,
                                          ble_hid_dsc_disc_cb,
                                          h);
        if (rc == 0) {
            return;
        }

        h->ble_ctx.discover_idx++;
    }

    ble_hid_subscribe_next(h);
}

static int ble_hid_svc_disc_cb(uint16_t conn_handle,
                                const struct ble_gatt_error *error,
                                const struct ble_gatt_svc *service,
                                void *arg)
{
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;

    if (error->status == BLE_HS_EDONE) {
        if (h->ble_ctx.hid_svc_start != 0) {
            ble_gattc_disc_all_chrs(conn_handle,
                                     h->ble_ctx.hid_svc_start,
                                     h->ble_ctx.hid_svc_end,
                                     ble_hid_chr_disc_cb, h);
        }
        return 0;
    }
    if (error->status != 0) {
        return 0;
    }
    if (ble_uuid_u16(&service->uuid.u) == BLE_HID_SVC_UUID16) {
        h->ble_ctx.hid_svc_start = service->start_handle;
        h->ble_ctx.hid_svc_end   = service->end_handle;
    }
    return 0;
}

static int ble_hid_subscribe_cb(uint16_t conn_handle,
                                  const struct ble_gatt_error *error,
                                  struct ble_gatt_attr *attr,
                                  void *arg)
{
    (void)error;
    (void)attr;
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;
    ble_hid_subscribe_next(h);
    (void)conn_handle;
    return 0;
}

static void ble_hid_subscribe_next(blusys_usb_hid_t *h)
{
    while (h->ble_ctx.subscribe_idx < h->ble_ctx.num_report_chrs) {
        int      idx         = h->ble_ctx.subscribe_idx++;
        uint16_t cccd_handle = h->ble_ctx.cccd_handles[idx];
        if (cccd_handle == 0) {
            continue;
        }
        uint16_t cccd_val    = htole16(0x0001);
        int rc = ble_gattc_write_flat(h->ble_ctx.conn_handle, cccd_handle,
                                       &cccd_val, sizeof(cccd_val),
                                       ble_hid_subscribe_cb, h);
        if (rc == 0) {
            return; /* Wait for callback before writing next CCCD */
        }
        /* Write error — skip to next */
    }

    /* All subscriptions done — connection is active */
    if (h->callback != NULL) {
        blusys_usb_hid_event_data_t ev = { .event = BLUSYS_USB_HID_EVENT_CONNECTED };
        h->callback(h, &ev, h->user_ctx);
    }
}

static int ble_hid_gap_event(struct ble_gap_event *event, void *arg)
{
    blusys_usb_hid_t *h = (blusys_usb_hid_t *)arg;

    switch (event->type) {

    case BLE_GAP_EVENT_DISC: {
        /* Filter by device name if configured */
        if (h->ble_device_name[0] != '\0') {
            struct ble_hs_adv_fields fields;
            if (ble_hs_adv_parse_fields(&fields,
                                         event->disc.data,
                                         event->disc.length_data) != 0) {
                return 0;
            }
            if (fields.name == NULL) {
                return 0;
            }
            size_t name_len = strlen(h->ble_device_name);
            if (fields.name_len != name_len ||
                memcmp(fields.name, h->ble_device_name, name_len) != 0) {
                return 0;
            }
        }

        /* Check advertisement for HID service UUID */
        if (!ble_hid_adv_has_hid_svc(event->disc.data, event->disc.length_data)) {
            return 0;
        }

        /* Found a matching HID device — stop scan and connect */
        ble_gap_disc_cancel();
        memset(&h->ble_ctx, 0, sizeof(h->ble_ctx));
        h->ble_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE;
        h->ble_ctx.last_report_idx = -1;

        ble_gap_connect(h->ble_own_addr_type, &event->disc.addr,
                        BLE_HS_FOREVER, NULL, ble_hid_gap_event, h);
        break;
    }

    case BLE_GAP_EVENT_DISC_COMPLETE:
        if (!h->ble_connected && !h->stopping) {
            ble_hid_start_scan(h);
        }
        break;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            h->ble_ctx.conn_handle = event->connect.conn_handle;
            h->ble_connected       = true;

            /* Discover HID service */
            ble_uuid16_t hid_svc_uuid;
            hid_svc_uuid.u.type = BLE_UUID_TYPE_16;
            hid_svc_uuid.value  = BLE_HID_SVC_UUID16;
            ble_gattc_disc_svc_by_uuid(event->connect.conn_handle,
                                        &hid_svc_uuid.u,
                                        ble_hid_svc_disc_cb, h);
        } else {
            /* Connection failed — restart scan */
            if (!h->stopping) {
                ble_hid_start_scan(h);
            }
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        h->ble_connected       = false;
        h->ble_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE;

        if (h->callback != NULL) {
            blusys_usb_hid_event_data_t ev = { .event = BLUSYS_USB_HID_EVENT_DISCONNECTED };
            h->callback(h, &ev, h->user_ctx);
        }
        if (!h->stopping) {
            ble_hid_start_scan(h);
        }
        break;

    case BLE_GAP_EVENT_NOTIFY_RX: {
        uint8_t  buf[64];
        uint16_t len = OS_MBUF_PKTLEN(event->notify_rx.om);
        if (len > sizeof(buf)) {
            len = sizeof(buf);
        }
        os_mbuf_copydata(event->notify_rx.om, 0, len, buf);
        dispatch_raw_report(h, buf, len);
        break;
    }

    default:
        break;
    }

    return 0;
}

static blusys_err_t ble_transport_open(blusys_usb_hid_t *h,
                                        const blusys_usb_hid_config_t *config)
{
    blusys_err_t err;
    esp_err_t    esp_err;

    if (config->ble_device_name != NULL) {
        strncpy(h->ble_device_name, config->ble_device_name,
                sizeof(h->ble_device_name) - 1);
    }

    h->ble_sync_event = xEventGroupCreate();
    if (h->ble_sync_event == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_nvs_ensure_init();
    if (err != BLUSYS_OK) {
        vEventGroupDelete(h->ble_sync_event);
        return err;
    }

    err = blusys_bt_stack_acquire(BLUSYS_BT_STACK_OWNER_USB_HID_BLE);
    if (err != BLUSYS_OK) {
        vEventGroupDelete(h->ble_sync_event);
        return err;
    }

    esp_err = nimble_port_init();
    if (esp_err != ESP_OK) {
        vEventGroupDelete(h->ble_sync_event);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_USB_HID_BLE);
        return blusys_translate_esp_err(esp_err);
    }

    ble_hs_cfg.sync_cb  = on_ble_hid_sync;
    ble_hs_cfg.reset_cb = on_ble_hid_reset;

    nimble_port_freertos_init(ble_hid_host_task);

    EventBits_t bits = xEventGroupWaitBits(h->ble_sync_event, BLE_HID_SYNC_BIT,
                                            pdFALSE, pdFALSE,
                                            pdMS_TO_TICKS(5000));
    if (!(bits & BLE_HID_SYNC_BIT)) {
        nimble_port_stop();
        nimble_port_deinit();
        vEventGroupDelete(h->ble_sync_event);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_USB_HID_BLE);
        return BLUSYS_ERR_TIMEOUT;
    }

    int rc = ble_hs_id_infer_auto(0, &h->ble_own_addr_type);
    if (rc != 0) {
        nimble_port_stop();
        nimble_port_deinit();
        vEventGroupDelete(h->ble_sync_event);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_USB_HID_BLE);
        return BLUSYS_ERR_INTERNAL;
    }

    rc = ble_hid_start_scan(h);
    if (rc != 0) {
        nimble_port_stop();
        nimble_port_deinit();
        vEventGroupDelete(h->ble_sync_event);
        blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_USB_HID_BLE);
        return BLUSYS_ERR_INTERNAL;
    }

    return BLUSYS_OK;
}

static blusys_err_t ble_transport_close(blusys_usb_hid_t *h)
{
    h->stopping = true;

    if (h->ble_connected) {
        ble_gap_terminate(h->ble_ctx.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    } else {
        ble_gap_disc_cancel();
    }

    nimble_port_stop();
    nimble_port_deinit();
    vEventGroupDelete(h->ble_sync_event);
    blusys_bt_stack_release(BLUSYS_BT_STACK_OWNER_USB_HID_BLE);
    return BLUSYS_OK;
}

#endif /* HAS_BLE_TRANSPORT */

/* ═══════════════════════════════════════════════════════════════════
 * Public API
 * ═══════════════════════════════════════════════════════════════════ */
blusys_err_t blusys_usb_hid_open(const blusys_usb_hid_config_t *config,
                                  blusys_usb_hid_t **out_hid)
{
    blusys_err_t err;

    if (config == NULL || out_hid == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

#if !HAS_USB_TRANSPORT
    if (config->transport == BLUSYS_USB_HID_TRANSPORT_USB) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }
#endif

#if !HAS_BLE_TRANSPORT
    if (config->transport == BLUSYS_USB_HID_TRANSPORT_BLE) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }
#endif

    err = ensure_global_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_usb_hid_handle != NULL) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_usb_hid_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_NO_MEM;
    }

    h->callback  = config->callback;
    h->user_ctx  = config->user_ctx;
    h->transport = config->transport;

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_hid_global_lock);
        free(h);
        return err;
    }

    s_usb_hid_handle = h;
    blusys_lock_give(&s_hid_global_lock);

    /* Open transport — lock released before blocking calls */
    switch (config->transport) {
#if HAS_USB_TRANSPORT
    case BLUSYS_USB_HID_TRANSPORT_USB:
        err = usb_transport_open(h);
        break;
#endif
#if HAS_BLE_TRANSPORT
    case BLUSYS_USB_HID_TRANSPORT_BLE:
        err = ble_transport_open(h, config);
        break;
#endif
    default:
        err = BLUSYS_ERR_NOT_SUPPORTED;
        break;
    }

    if (err != BLUSYS_OK) {
        if (blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
            s_usb_hid_handle = NULL;
            blusys_lock_give(&s_hid_global_lock);
        }
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    *out_hid = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_usb_hid_close(blusys_usb_hid_t *hid)
{
    blusys_err_t err;

    if (hid == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&s_hid_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_usb_hid_handle != hid) {
        blusys_lock_give(&s_hid_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }

    s_usb_hid_handle = NULL;
    blusys_lock_give(&s_hid_global_lock);

    hid->stopping = true;

    switch (hid->transport) {
#if HAS_USB_TRANSPORT
    case BLUSYS_USB_HID_TRANSPORT_USB:
        usb_transport_close(hid);
        break;
#endif
#if HAS_BLE_TRANSPORT
    case BLUSYS_USB_HID_TRANSPORT_BLE:
        ble_transport_close(hid);
        break;
#endif
    default:
        break;
    }

    blusys_lock_deinit(&hid->lock);
    free(hid);
    return BLUSYS_OK;
}

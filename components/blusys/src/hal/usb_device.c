#include "blusys/hal/usb_device.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_USB_OTG_SUPPORTED && defined(BLUSYS_HAS_TINYUSB)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "class/hid/hid_device.h"

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#define USB_DEVICE_CDC_RX_BUF_SIZE 512

struct blusys_usb_device {
    blusys_lock_t                        lock;
    blusys_usb_device_class_t            device_class;
    blusys_usb_device_event_callback_t   event_cb;
    void                                *event_user_ctx;
    /* CDC-specific */
    blusys_usb_device_cdc_rx_callback_t  cdc_rx_cb;
    void                                *cdc_rx_user_ctx;
};

static portMUX_TYPE        s_dev_init_lock = portMUX_INITIALIZER_UNLOCKED;
static blusys_lock_t       s_dev_global_lock;
static bool                s_dev_global_lock_inited;
static blusys_usb_device_t *s_usb_device_handle;

static blusys_err_t ensure_global_lock(void)
{
    if (s_dev_global_lock_inited) {
        return BLUSYS_OK;
    }
    blusys_lock_t new_lock;
    blusys_err_t err = blusys_lock_init(&new_lock);
    if (err != BLUSYS_OK) {
        return err;
    }
    portENTER_CRITICAL(&s_dev_init_lock);
    if (!s_dev_global_lock_inited) {
        s_dev_global_lock       = new_lock;
        s_dev_global_lock_inited = true;
        portEXIT_CRITICAL(&s_dev_init_lock);
    } else {
        portEXIT_CRITICAL(&s_dev_init_lock);
        blusys_lock_deinit(&new_lock);
    }
    return BLUSYS_OK;
}

/* CDC ACM receive callback — called from TinyUSB task */
static void tusb_cdc_rx_cb(int itf, cdcacm_event_t *event)
{
    (void)itf;
    (void)event;

    if (blusys_lock_take(&s_dev_global_lock, 0) != BLUSYS_OK) {
        return;
    }
    blusys_usb_device_t *h = s_usb_device_handle;
    if (h == NULL || h->cdc_rx_cb == NULL) {
        blusys_lock_give(&s_dev_global_lock);
        return;
    }
    blusys_usb_device_cdc_rx_callback_t cb = h->cdc_rx_cb;
    void *user_ctx = h->cdc_rx_user_ctx;
    blusys_lock_give(&s_dev_global_lock);

    /* Read all available bytes and forward to the user callback */
    uint8_t buf[USB_DEVICE_CDC_RX_BUF_SIZE];
    size_t rx_len = 0;
    tinyusb_cdcacm_read(itf, buf, sizeof(buf), &rx_len);
    if (rx_len > 0 && cb != NULL) {
        /* Lock not held across callback — consistent with blusys rule */
        cb(h, buf, rx_len, user_ctx);
    }
}

blusys_err_t blusys_usb_device_open(const blusys_usb_device_config_t *config,
                                     blusys_usb_device_t **out_dev)
{
    blusys_err_t err;
    esp_err_t    esp_err;

    if (config == NULL || out_dev == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if (config->device_class == BLUSYS_USB_DEVICE_CLASS_MSC) {
        return BLUSYS_ERR_NOT_SUPPORTED;
    }

    err = ensure_global_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_dev_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_usb_device_handle != NULL) {
        blusys_lock_give(&s_dev_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_usb_device_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_dev_global_lock);
        return BLUSYS_ERR_NO_MEM;
    }

    h->device_class    = config->device_class;
    h->event_cb        = config->event_cb;
    h->event_user_ctx  = config->event_user_ctx;

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        blusys_lock_give(&s_dev_global_lock);
        free(h);
        return err;
    }

    /* Build device descriptor strings */
    const char *str_desc[] = {
        /* 0: Language */    "\x09\x04",
        /* 1: Manufacturer */ config->manufacturer ? config->manufacturer : "Blusys",
        /* 2: Product */      config->product      ? config->product      : "USB Device",
        /* 3: Serial */       config->serial       ? config->serial       : "1234567890",
    };

    tinyusb_config_t tusb_cfg = {
        .device_descriptor      = NULL, /* TinyUSB built-in default descriptor */
        .string_descriptor      = (const char **)str_desc,
        .string_descriptor_count = 4,
        .external_phy           = false,
        .configuration_descriptor = NULL,
    };

    esp_err = tinyusb_driver_install(&tusb_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&h->lock);
        blusys_lock_give(&s_dev_global_lock);
        free(h);
        return err;
    }

    if (config->device_class == BLUSYS_USB_DEVICE_CLASS_CDC) {
        tinyusb_config_cdcacm_t acm_cfg = {
            .usb_dev      = TINYUSB_USBDEV_0,
            .callback_rx  = tusb_cdc_rx_cb,
            .callback_rx_wanted_char = NULL,
            .callback_line_state_changed = NULL,
            .callback_line_coding_changed = NULL,
        };
        esp_err = tusb_cdc_acm_init(&acm_cfg);
        if (esp_err != ESP_OK) {
            err = blusys_translate_esp_err(esp_err);
            tinyusb_driver_uninstall();
            blusys_lock_deinit(&h->lock);
            blusys_lock_give(&s_dev_global_lock);
            free(h);
            return err;
        }
    }

    s_usb_device_handle = h;
    blusys_lock_give(&s_dev_global_lock);

    *out_dev = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_usb_device_close(blusys_usb_device_t *dev)
{
    blusys_err_t err;

    if (dev == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&s_dev_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_usb_device_handle != dev) {
        blusys_lock_give(&s_dev_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }

    s_usb_device_handle = NULL;
    blusys_lock_give(&s_dev_global_lock);

    tinyusb_driver_uninstall();

    blusys_lock_deinit(&dev->lock);
    free(dev);
    return BLUSYS_OK;
}

blusys_err_t blusys_usb_device_cdc_write(blusys_usb_device_t *dev,
                                          const void *data, size_t len)
{
    blusys_err_t err;
    esp_err_t    esp_err;

    if (dev == NULL || data == NULL || len == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dev->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (dev->device_class != BLUSYS_USB_DEVICE_CLASS_CDC) {
        blusys_lock_give(&dev->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_err = tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (uint8_t *)data, len);
    if (esp_err == ESP_OK) {
        esp_err = tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    }
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&dev->lock);
    return err;
}

blusys_err_t blusys_usb_device_cdc_set_rx_callback(
    blusys_usb_device_t *dev,
    blusys_usb_device_cdc_rx_callback_t callback,
    void *user_ctx)
{
    blusys_err_t err;

    if (dev == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dev->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (dev->device_class != BLUSYS_USB_DEVICE_CLASS_CDC) {
        blusys_lock_give(&dev->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    dev->cdc_rx_cb       = callback;
    dev->cdc_rx_user_ctx = user_ctx;

    blusys_lock_give(&dev->lock);
    return BLUSYS_OK;
}

blusys_err_t blusys_usb_device_hid_send_report(blusys_usb_device_t *dev,
                                                 const uint8_t *report,
                                                 size_t len)
{
    blusys_err_t err;

    if (dev == NULL || report == NULL || len == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&dev->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (dev->device_class != BLUSYS_USB_DEVICE_CLASS_HID) {
        blusys_lock_give(&dev->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    bool ok = tud_hid_report(0, report, (uint16_t)len);
    blusys_lock_give(&dev->lock);

    return ok ? BLUSYS_OK : BLUSYS_ERR_INTERNAL;
}

#else /* !(SOC_USB_OTG_SUPPORTED && BLUSYS_HAS_TINYUSB) */

blusys_err_t blusys_usb_device_open(const blusys_usb_device_config_t *config,
                                     blusys_usb_device_t **out_dev)
{
    (void)config;
    (void)out_dev;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_usb_device_close(blusys_usb_device_t *dev)
{
    (void)dev;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_usb_device_cdc_write(blusys_usb_device_t *dev,
                                          const void *data, size_t len)
{
    (void)dev;
    (void)data;
    (void)len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_usb_device_cdc_set_rx_callback(
    blusys_usb_device_t *dev,
    blusys_usb_device_cdc_rx_callback_t callback,
    void *user_ctx)
{
    (void)dev;
    (void)callback;
    (void)user_ctx;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_usb_device_hid_send_report(blusys_usb_device_t *dev,
                                                 const uint8_t *report,
                                                 size_t len)
{
    (void)dev;
    (void)report;
    (void)len;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_USB_OTG_SUPPORTED && BLUSYS_HAS_TINYUSB */

#include "blusys/hal/usb_host.h"

#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_USB_OTG_SUPPORTED

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "usb/usb_host.h"

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"

#define USB_HOST_DAEMON_TASK_PRIORITY  2
#define USB_HOST_CLIENT_TASK_PRIORITY  2
#define USB_HOST_TASK_STACK_SIZE       4096
#define USB_HOST_CLIENT_NUM_EVENT_MSG  5

#define USB_HOST_DAEMON_STOP_BIT       BIT0
#define USB_HOST_CLIENT_STOP_BIT       BIT1

struct blusys_usb_host {
    blusys_lock_t                      lock;
    usb_host_client_handle_t           client_hdl;
    blusys_usb_host_event_callback_t   event_cb;
    void                              *event_user_ctx;
    TaskHandle_t                       daemon_task;
    TaskHandle_t                       client_task;
    EventGroupHandle_t                 stop_event;
    volatile bool                      stopping;
};

static portMUX_TYPE       s_host_init_lock = portMUX_INITIALIZER_UNLOCKED;
static blusys_lock_t      s_host_global_lock;
static bool               s_host_global_lock_inited;
static blusys_usb_host_t *s_usb_host_handle;

static blusys_err_t ensure_global_lock(void)
{
    if (s_host_global_lock_inited) {
        return BLUSYS_OK;
    }
    blusys_lock_t new_lock;
    blusys_err_t err = blusys_lock_init(&new_lock);
    if (err != BLUSYS_OK) {
        return err;
    }
    portENTER_CRITICAL(&s_host_init_lock);
    if (!s_host_global_lock_inited) {
        s_host_global_lock       = new_lock;
        s_host_global_lock_inited = true;
        portEXIT_CRITICAL(&s_host_init_lock);
    } else {
        portEXIT_CRITICAL(&s_host_init_lock);
        blusys_lock_deinit(&new_lock);
    }
    return BLUSYS_OK;
}

static void usb_host_daemon_task(void *arg)
{
    blusys_usb_host_t *h = (blusys_usb_host_t *)arg;
    uint32_t event_flags;

    while (true) {
        esp_err_t ret = usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (ret != ESP_OK) {
            break;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            break;
        }
    }

    xEventGroupSetBits(h->stop_event, USB_HOST_DAEMON_STOP_BIT);
    vTaskDelete(NULL);
}

static void usb_host_client_event_cb(const usb_host_client_event_msg_t *msg, void *arg)
{
    blusys_usb_host_t *h = (blusys_usb_host_t *)arg;

    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        uint8_t dev_addr = msg->new_dev.address;
        usb_device_handle_t dev_hdl;
        const usb_device_desc_t *desc;

        if (usb_host_device_open(h->client_hdl, dev_addr, &dev_hdl) != ESP_OK) {
            return;
        }
        if (usb_host_get_device_descriptor(dev_hdl, &desc) == ESP_OK && h->event_cb != NULL) {
            blusys_usb_host_device_info_t info = {
                .vid      = desc->idVendor,
                .pid      = desc->idProduct,
                .dev_addr = dev_addr,
            };
            /* Never hold a lock across the user callback */
            h->event_cb(h, BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED, &info, h->event_user_ctx);
        }
        usb_host_device_close(h->client_hdl, dev_hdl);

    } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        if (h->event_cb != NULL) {
            blusys_usb_host_device_info_t info = { 0 };
            h->event_cb(h, BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED, &info, h->event_user_ctx);
        }
    }
}

static void usb_host_client_task(void *arg)
{
    blusys_usb_host_t *h = (blusys_usb_host_t *)arg;

    while (!h->stopping) {
        esp_err_t ret = usb_host_client_handle_events(h->client_hdl, pdMS_TO_TICKS(200));
        if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
            break;
        }
    }

    xEventGroupSetBits(h->stop_event, USB_HOST_CLIENT_STOP_BIT);
    vTaskDelete(NULL);
}

blusys_err_t blusys_usb_host_open(const blusys_usb_host_config_t *config,
                                   blusys_usb_host_t **out_host)
{
    blusys_err_t err;
    esp_err_t    esp_err;

    if (config == NULL || out_host == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = ensure_global_lock();
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_lock_take(&s_host_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_usb_host_handle != NULL) {
        blusys_lock_give(&s_host_global_lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    blusys_usb_host_t *h = calloc(1, sizeof(*h));
    if (h == NULL) {
        blusys_lock_give(&s_host_global_lock);
        return BLUSYS_ERR_NO_MEM;
    }

    h->event_cb       = config->event_cb;
    h->event_user_ctx = config->event_user_ctx;

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        goto fail_alloc;
    }

    h->stop_event = xEventGroupCreate();
    if (h->stop_event == NULL) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_lock;
    }

    usb_host_config_t host_cfg = {
        .skip_phy_setup = false,
        .intr_flags     = ESP_INTR_FLAG_LEVEL1,
    };
    esp_err = usb_host_install(&host_cfg);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_event;
    }

    usb_host_client_config_t client_cfg = {
        .is_synchronous    = false,
        .max_num_event_msg = USB_HOST_CLIENT_NUM_EVENT_MSG,
        .async = {
            .client_event_callback = usb_host_client_event_cb,
            .callback_arg          = h,
        },
    };
    esp_err = usb_host_client_register(&client_cfg, &h->client_hdl);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        goto fail_host;
    }

    s_usb_host_handle = h;
    blusys_lock_give(&s_host_global_lock);

    /* Start tasks after releasing the global lock (never block while holding it) */
    BaseType_t ret;
    ret = xTaskCreate(usb_host_daemon_task, "usb_daemon",
                      USB_HOST_TASK_STACK_SIZE, h,
                      USB_HOST_DAEMON_TASK_PRIORITY, &h->daemon_task);
    if (ret != pdPASS) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_tasks;
    }

    ret = xTaskCreate(usb_host_client_task, "usb_client",
                      USB_HOST_TASK_STACK_SIZE, h,
                      USB_HOST_CLIENT_TASK_PRIORITY, &h->client_task);
    if (ret != pdPASS) {
        err = BLUSYS_ERR_NO_MEM;
        goto fail_tasks;
    }

    *out_host = h;
    return BLUSYS_OK;

fail_tasks:
    h->stopping = true;
    usb_host_client_deregister(h->client_hdl);
    /* Only wait for daemon stop bit if the daemon task was actually created.
     * If daemon creation failed first, no task exists to set any bit. */
    if (h->daemon_task != NULL) {
        xEventGroupWaitBits(h->stop_event, USB_HOST_DAEMON_STOP_BIT,
                            pdFALSE, pdTRUE, portMAX_DELAY);
    }
    usb_host_uninstall();
    if (blusys_lock_take(&s_host_global_lock, BLUSYS_LOCK_WAIT_FOREVER) == BLUSYS_OK) {
        s_usb_host_handle = NULL;
        blusys_lock_give(&s_host_global_lock);
    }
    vEventGroupDelete(h->stop_event);
    blusys_lock_deinit(&h->lock);
    free(h);
    return err;

fail_host:
    usb_host_uninstall();
fail_event:
    vEventGroupDelete(h->stop_event);
fail_lock:
    blusys_lock_deinit(&h->lock);
fail_alloc:
    blusys_lock_give(&s_host_global_lock);
    free(h);
    return err;
}

blusys_err_t blusys_usb_host_close(blusys_usb_host_t *host)
{
    blusys_err_t err;

    if (host == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&s_host_global_lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (s_usb_host_handle != host) {
        blusys_lock_give(&s_host_global_lock);
        return BLUSYS_ERR_INVALID_ARG;
    }

    host->stopping    = true;
    s_usb_host_handle = NULL;
    blusys_lock_give(&s_host_global_lock);

    /* Deregister the client — triggers USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS in daemon */
    usb_host_client_deregister(host->client_hdl);

    /* Wait for both tasks to exit */
    xEventGroupWaitBits(host->stop_event,
                        USB_HOST_DAEMON_STOP_BIT | USB_HOST_CLIENT_STOP_BIT,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    usb_host_uninstall();
    vEventGroupDelete(host->stop_event);
    blusys_lock_deinit(&host->lock);
    free(host);
    return BLUSYS_OK;
}

#else /* !SOC_USB_OTG_SUPPORTED */

blusys_err_t blusys_usb_host_open(const blusys_usb_host_config_t *config,
                                   blusys_usb_host_t **out_host)
{
    (void)config;
    (void)out_host;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_usb_host_close(blusys_usb_host_t *host)
{
    (void)host;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_USB_OTG_SUPPORTED */

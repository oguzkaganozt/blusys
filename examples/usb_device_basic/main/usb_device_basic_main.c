#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

static blusys_usb_device_t *s_dev;

static void on_cdc_rx(blusys_usb_device_t *dev,
                       const uint8_t *data,
                       size_t len,
                       void *user_ctx)
{
    (void)user_ctx;
    /* Echo received data back to the host */
    blusys_usb_device_cdc_write(dev, data, len);
}

static void on_usb_event(blusys_usb_device_t *dev,
                          blusys_usb_device_event_t event,
                          void *user_ctx)
{
    (void)dev;
    (void)user_ctx;

    switch (event) {
    case BLUSYS_USB_DEVICE_EVENT_CONNECTED:
        printf("host connected\n");
        break;
    case BLUSYS_USB_DEVICE_EVENT_DISCONNECTED:
        printf("host disconnected\n");
        break;
    case BLUSYS_USB_DEVICE_EVENT_SUSPENDED:
        printf("suspended\n");
        break;
    case BLUSYS_USB_DEVICE_EVENT_RESUMED:
        printf("resumed\n");
        break;
    }
}

void app_main(void)
{
    printf("Blusys usb_device basic (CDC echo)\n");
    printf("target: %s\n", blusys_target_name());
    printf("usb_device supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_USB_DEVICE) ? "yes" : "no");

    blusys_usb_device_config_t cfg = {
        .vid          = 0x303A,       /* Espressif VID */
        .pid          = 0x4001,
        .manufacturer = "Blusys",
        .product      = "CDC Echo",
        .serial       = "1234567890",
        .device_class = BLUSYS_USB_DEVICE_CLASS_CDC,
        .event_cb     = on_usb_event,
    };

    blusys_err_t err = blusys_usb_device_open(&cfg, &s_dev);
    if (err != BLUSYS_OK) {
        printf("usb_device_open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("usb_device_open: ok — connect a USB cable to the host PC\n");

    blusys_usb_device_cdc_set_rx_callback(s_dev, on_cdc_rx, NULL);

    /* Run for 60 seconds then clean up */
    vTaskDelay(pdMS_TO_TICKS(60000));

    err = blusys_usb_device_close(s_dev);
    if (err != BLUSYS_OK) {
        printf("usb_device_close failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("done\n");
}

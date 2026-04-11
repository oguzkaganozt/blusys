#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/blusys.h"

static void on_usb_event(blusys_usb_host_t *host,
                          blusys_usb_host_event_t event,
                          const blusys_usb_host_device_info_t *device,
                          void *user_ctx)
{
    (void)host;
    (void)user_ctx;

    switch (event) {
    case BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED:
        printf("device connected: addr=%u  VID=0x%04X  PID=0x%04X\n",
               device->dev_addr, device->vid, device->pid);
        break;
    case BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED:
        printf("device disconnected\n");
        break;
    }
}

void app_main(void)
{
    printf("Blusys usb_host basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("usb_host supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_USB_HOST) ? "yes" : "no");

    blusys_usb_host_config_t cfg = {
        .event_cb       = on_usb_event,
        .event_user_ctx = NULL,
    };

    blusys_usb_host_t *host = NULL;
    blusys_err_t err = blusys_usb_host_open(&cfg, &host);
    if (err != BLUSYS_OK) {
        printf("usb_host_open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("usb_host_open: ok — plug in a USB device\n");

    /* Run for 30 seconds then clean up */
    vTaskDelay(pdMS_TO_TICKS(30000));

    err = blusys_usb_host_close(host);
    if (err != BLUSYS_OK) {
        printf("usb_host_close failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("done\n");
}

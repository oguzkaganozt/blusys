#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "blusys/blusys.h"
#include "blusys/input/usb_hid.h"

#ifdef CONFIG_BLUSYS_USB_HID_BASIC_TRANSPORT_BLE
#define HID_TRANSPORT BLUSYS_USB_HID_TRANSPORT_BLE
#define HID_TRANSPORT_STR "BLE"
#else
#define HID_TRANSPORT BLUSYS_USB_HID_TRANSPORT_USB
#define HID_TRANSPORT_STR "USB"
#endif

static void on_hid_event(blusys_usb_hid_t *hid,
                          const blusys_usb_hid_event_data_t *event,
                          void *user_ctx)
{
    (void)hid;
    (void)user_ctx;

    switch (event->event) {
    case BLUSYS_USB_HID_EVENT_CONNECTED:
        printf("HID device connected\n");
        break;
    case BLUSYS_USB_HID_EVENT_DISCONNECTED:
        printf("HID device disconnected\n");
        break;
    case BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT:
        printf("keyboard: modifier=0x%02X  keys=[%02X %02X %02X %02X %02X %02X]\n",
               event->data.keyboard.modifier,
               event->data.keyboard.keycodes[0],
               event->data.keyboard.keycodes[1],
               event->data.keyboard.keycodes[2],
               event->data.keyboard.keycodes[3],
               event->data.keyboard.keycodes[4],
               event->data.keyboard.keycodes[5]);
        break;
    case BLUSYS_USB_HID_EVENT_MOUSE_REPORT:
        printf("mouse: buttons=0x%02X  x=%d  y=%d  wheel=%d\n",
               event->data.mouse.buttons,
               event->data.mouse.x,
               event->data.mouse.y,
               event->data.mouse.wheel);
        break;
    case BLUSYS_USB_HID_EVENT_RAW_REPORT:
        printf("raw report: %zu bytes\n", event->data.raw.len);
        break;
    }
}

void app_main(void)
{
    printf("Blusys usb_hid basic\n");
    printf("target: %s  transport: %s\n", blusys_target_name(), HID_TRANSPORT_STR);
    printf("usb_hid supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_USB_HID) ? "yes" : "no");

    blusys_usb_hid_config_t cfg = {
        .transport = HID_TRANSPORT,
        .callback  = on_hid_event,
        .user_ctx  = NULL,
#ifdef CONFIG_BLUSYS_USB_HID_BASIC_BLE_DEVICE_NAME
        .ble_device_name = CONFIG_BLUSYS_USB_HID_BASIC_BLE_DEVICE_NAME[0] != '\0'
                           ? CONFIG_BLUSYS_USB_HID_BASIC_BLE_DEVICE_NAME : NULL,
#else
        .ble_device_name = NULL,
#endif
    };

    blusys_usb_hid_t *hid = NULL;
    blusys_err_t err = blusys_usb_hid_open(&cfg, &hid);
    if (err != BLUSYS_OK) {
        printf("usb_hid_open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("usb_hid_open: ok — connect a HID device\n");

    /* Run for 60 seconds then clean up */
    vTaskDelay(pdMS_TO_TICKS(60000));

    err = blusys_usb_hid_close(hid);
    if (err != BLUSYS_OK) {
        printf("usb_hid_close failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("done\n");
}

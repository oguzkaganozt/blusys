#include "sdkconfig.h"

void run_usb_host_basic(void);
void run_usb_device_basic(void);
void run_usb_hid_basic(void);

void app_main(void)
{
#if CONFIG_USB_PERIPHERAL_LAB_SCENARIO_USB_HOST_BASIC
    run_usb_host_basic();
#elif CONFIG_USB_PERIPHERAL_LAB_SCENARIO_USB_DEVICE_BASIC
    run_usb_device_basic();
#elif CONFIG_USB_PERIPHERAL_LAB_SCENARIO_USB_HID_BASIC
    run_usb_hid_basic();
#endif
}

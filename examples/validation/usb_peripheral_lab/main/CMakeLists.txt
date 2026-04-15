idf_component_register(
    SRCS
        "usb_peripheral_lab_main.c"
        "usbpl_usb_host_basic.c"
        "usbpl_usb_device_basic.c"
        "usbpl_usb_hid_basic.c"
    REQUIRES blusys
)

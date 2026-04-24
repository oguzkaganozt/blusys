# Services

Runtime services sit above the HAL and below the framework capabilities. Use them when you need exact lifecycle or module-specific types.

!!! note
    Most product code should prefer [Capabilities](../app/capabilities.md) and only drop down to services when the direct API matters.

## Connectivity

- [WiFi](wifi.md)
- [WiFi Provisioning](wifi_prov.md)
- [WiFi Mesh](wifi_mesh.md)
- [ESP-NOW](espnow.md)
- [Bluetooth](bluetooth.md)
- [BLE GATT](ble_gatt.md)
- [BLE HID Device](ble_hid_device.md)
- [mDNS](mdns.md)

## Protocol

- [HTTP](http.md)
- [MQTT](mqtt.md)
- [WebSocket](ws_client.md)

## Display & Input

- [UI (LVGL)](ui.md)
- [USB HID](usb_hid.md)

## System

- [Console](console.md)
- [Filesystem](fs.md)
- [FAT Filesystem](fatfs.md)
- [Local Control](local_ctrl.md)
- [Power Management](power_mgmt.md)
- [SNTP](sntp.md)
- [OTA](ota.md)

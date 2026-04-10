# Services

Runtime services provide stateful capabilities on top of the HAL layer. For most product apps, use [service bundles](../app/service-bundles.md) instead of calling these APIs directly.

## Connectivity

| Service | Description |
|---------|-------------|
| [WiFi](wifi.md) | Station and AP mode connection management |
| [WiFi Provisioning](wifi_prov.md) | BLE or SoftAP-based credential provisioning |
| [WiFi Mesh](wifi_mesh.md) | ESP-MESH network formation |
| [ESP-NOW](espnow.md) | Connectionless peer-to-peer messaging |
| [Bluetooth](bluetooth.md) | Classic Bluetooth SPP profiles |
| [BLE GATT](ble_gatt.md) | BLE peripheral GATT server |
| [mDNS](mdns.md) | Multicast DNS service discovery |

## Protocol

| Service | Description |
|---------|-------------|
| [HTTP](http.md) | HTTP client and server |
| [MQTT](mqtt.md) | MQTT client with TLS support |
| [WebSocket](ws_client.md) | WebSocket client |

## Display & Input

| Service | Description |
|---------|-------------|
| [UI](ui.md) | LVGL widget system and display runtime |
| [USB HID](usb_hid.md) | USB and BLE HID input devices |

## System

| Service | Description |
|---------|-------------|
| [Console](console.md) | Serial console and command registration |
| [Filesystem](fs.md) | SPIFFS-based filesystem |
| [FAT Filesystem](fatfs.md) | FAT filesystem on flash or SD |
| [Local Control](local_ctrl.md) | Local network control endpoint |
| [Power Management](power_mgmt.md) | CPU frequency and sleep policy |
| [SNTP](sntp.md) | Network time synchronization |
| [OTA](ota.md) | Over-the-air firmware updates |

## Current Blusys Coverage (35 modules)

**Peripherals (24):** GPIO, UART, I2C, I2C Slave, SPI, SPI Slave, PWM/LEDC, ADC, DAC, Timer, PCNT, RMT, RMT RX, TWAI/CAN, I2S, I2S RX, Touch, SDMMC, Temp Sensor, WDT, Sleep, MCPWM, SDM

**Networking (9):** WiFi, MQTT, HTTP Client, HTTP Server, SNTP, mDNS, ESP-NOW, Bluetooth Classic, BLE GATT

**Storage/System (3):** NVS, FS (SPIFFS), OTA

---

## Missing from ESP-IDF v5.5.4

### High Relevance (commonly used, fits HAL scope)

| Module | ESP-IDF Component | Description |
|--------|-------------------|-------------|
| **Ethernet** | `esp_eth` | Wired networking (SPI-based like W5500, or native on ESP32) |
| **HTTPS Server** | `esp_https_server` | TLS-enabled HTTP server |
| **FatFS** | `fatfs` + `wear_levelling` | FAT filesystem for SD cards and flash (SPIFFS is limited) |
| **SD SPI** | `esp_driver_sdspi` | SD card over SPI bus (cheaper than SDMMC, works on all targets) |
| **Console/CLI** | `console` | Interactive UART console with command registration |
| **WiFi Provisioning** | `wifi_provisioning` | BLE/SoftAP-based WiFi credential setup |
| **Power Management** | `esp_pm` | CPU frequency scaling, light sleep automation |
| **Event Loop** | `esp_event` | Publish/subscribe event system |
| **ESP-TLS** | `esp-tls` | Unified TLS abstraction (used by MQTT, HTTP client, etc.) |
| **WebSocket Client** | `esp_websocket_client` (managed) | WebSocket protocol for real-time bidirectional communication |
| **LED Strip** | `led_strip` (managed) | Addressable LED support (WS2812, SK6812, etc.) |
| **USB Host/Device** | `usb` | Full USB host stack with hub support and device classes |
| **HID** | `esp_hid` | Human Interface Device over BLE/BT (keyboards, mice, gamepads) |
| **WiFi Mesh** | `esp_mesh` (in `esp_wifi`) | Self-healing mesh networking for large-scale IoT deployments |
| **SmartConfig** | `esp_smartconfig` (in `esp_wifi`) | Mobile app WiFi provisioning (ESPTouch/AirKiss), separate from wifi_provisioning |

### Medium Relevance (useful for specific applications)

| Module | ESP-IDF Component | Description |
|--------|-------------------|-------------|
| **LCD/Display** | `esp_lcd` | SPI/I2C/parallel display drivers |
| **Camera** | `esp_driver_cam` | Camera interface (ESP32-S3 DVP/MIPI) |
| **USB Serial JTAG** | `esp_driver_usb_serial_jtag` | USB CDC/JTAG (ESP32-S3, C3) |
| **Analog Comparator** | `esp_driver_ana_cmpr` | Analog signal comparison (C3, S3) |
| **Parallel I/O** | `esp_driver_parlio` | Parallel data interface (S3) |
| **Touch Element** | `touch_element` | Higher-level touch (buttons, sliders, matrices) |
| **ULP** | `ulp` | Ultra Low Power coprocessor programming |
| **Local Control** | `esp_local_ctrl` | Local device control over BLE/WiFi |
| **eFuse** | `efuse` | One-time programmable memory reading |
| **OpenThread** | `openthread` | Thread mesh networking (C3, S3) |
| **Button** | `button` (managed) | Debouncing, long-press, and event handling abstraction |
| **GPIO Expander** | `esp_io_expander` (managed) | I2C/SPI-based port expanders |
| **CoAP** | `coap` (managed) | Constrained Application Protocol for lightweight IoT |
| **Encrypted OTA** | `esp_encrypted_img` (managed) | Encrypted firmware image support for secure OTA |
| **Secure Boot** | `bootloader_support` | Firmware signature verification chain for production devices |
| **Hardware Crypto** | `esp_hw_support` (esp_ds) | Hardware-accelerated RSA/AES/SHA operations |
| **Partition Manager** | `esp_partition` | Read/write/find partitions in flash (needed for advanced OTA, custom data) |
| **App Trace** | `app_trace` | JTAG-based tracing and performance profiling |

### Low Relevance (niche, internal, or out of HAL scope)

| Module | Why low priority |
|--------|-----------------|
| JPEG codec | Very specialized (S3 only) |
| ISP (Image Signal Processor) | Camera pipeline specific |
| Bit Scrambler | Niche security feature |
| PPA (Pixel Processing Accelerator) | Graphics-specific (S3) |
| SDIO | Rare use case |
| Protocomm | Internal to provisioning |
| IEEE 802.15.4 raw | Covered if OpenThread is added |
| TinyUSB (managed) | USB class drivers (CDC, MSC, HID) — overlaps USB Host/Device |
| Key Manager | Hardware key storage — very niche security use case |
| RTC Time | Low-power RTC counter — mostly covered by Timer + Sleep modules |
| Ring Buffer | FreeRTOS utility — too low-level for HAL |
| DMA Utilities | Internal memory allocation — too low-level for HAL |
| Core Dump (`espcoredump`) | Crash dump analysis — debugging tool, not a peripheral |
| NVS Encryption (`nvs_sec_provider`) | Encrypted NVS — niche security extension of existing NVS |
| Performance Monitor (`perfmon`) | CPU performance counters — debugging/profiling tool |
| SPI Flash Direct (`spi_flash`) | Raw flash read/write — most use cases covered by NVS/FS/Partition Manager |

---

## Summary

Blusys covers **~70% of what most ESP32 projects need**. The biggest gaps are:

1. **Ethernet** — essential for wired IoT applications
2. **FatFS + SD SPI** — SPIFFS is deprecated/limited; many projects need FAT on SD cards
3. **Console** — very common for debugging and device configuration
4. **WiFi Provisioning** — standard for consumer products
5. **Power Management** — critical for battery-powered devices
6. **ESP-TLS** — currently your MQTT/HTTP modules likely use it internally, but there's no direct API for custom secure sockets
7. **LCD/Display** — very popular use case, especially with ESP32-S3
8. **Event Loop** — foundational pattern in ESP-IDF that many apps use directly
9. **USB Host/Device** — full USB stack for peripherals (S3 has native USB-OTG)
10. **WebSocket Client** — real-time bidirectional communication, common in IoT dashboards
11. **LED Strip** — addressable LEDs (WS2812) are ubiquitous in ESP32 projects
12. **HID** — BLE/BT keyboards, mice, gamepads — popular for input devices
13. **WiFi Mesh** — self-healing mesh for large-scale IoT deployments
14. **SmartConfig** — mobile app provisioning (ESPTouch), very common in consumer products

# Blusys Feature Coverage & Roadmap

## Current Coverage — V1 through V3 (36 modules)

**Peripherals (23):** GPIO, UART, I2C, I2C Slave, SPI, SPI Slave, PWM/LEDC, ADC, DAC, Timer, PCNT, RMT, RMT RX, TWAI/CAN, I2S, I2S RX, Touch, SDMMC, Temp Sensor, WDT, Sleep, MCPWM, SDM

**Networking (9):** WiFi, MQTT, HTTP Client, HTTP Server, SNTP, mDNS, ESP-NOW, Bluetooth Classic, BLE GATT

**Storage/System (4):** System, NVS, FS (SPIFFS), OTA

---

## V4 — Production Essentials (9 modules)

Everything needed to take a blusys device from prototype to shippable product.

| # | Module | Category | ESP-IDF Component | Description |
|---|--------|----------|-------------------|-------------|
| 1 | **FatFS** | Storage | `fatfs` + `wear_levelling` | FAT filesystem for SD cards and flash (replaces SPIFFS limitations) |
| 2 | **SD SPI** | Storage | `esp_driver_sdspi` | SD card over SPI bus — cheaper than SDMMC, works on all targets |
| 3 | **WiFi Provisioning** | Provisioning | `wifi_provisioning` | BLE/SoftAP-based WiFi credential setup |
| 4 | **Console/CLI** | DevTools | `console` | Interactive UART console with command registration |
| 5 | **Power Management** | Power | `esp_pm` | CPU frequency scaling, light sleep automation |
| 6 | **WebSocket Client** | Networking | `esp_websocket_client` (managed) | Real-time bidirectional communication for IoT dashboards |
| 7 | **LED Strip** | Peripheral | `led_strip` (managed) | Addressable LED support (WS2812, SK6812, etc.) |
| 8 | **Button** | Peripheral | `button` (managed) | Debouncing, long-press, and event handling abstraction |
| 9 | **LCD/Display** | Peripheral | `esp_lcd` | SPI/I2C/parallel display drivers |

**Theme:** Storage, provisioning, power, real-time comms, display, and input — everything to ship a real product.

---

## V5 — Advanced Connectivity & Peripherals (9 modules)

Specialized peripherals and extended networking for advanced use cases.

| # | Module | Category | ESP-IDF Component | Description |
|---|--------|----------|-------------------|-------------|
| 1 | **WiFi Mesh** | Networking | `esp_mesh_lite` (managed) | Lightweight mesh networking for large-scale IoT deployments |
| 2 | **Ethernet** | Networking | `esp_eth` | Wired networking (SPI-based like W5500, or native on ESP32) |
| 3 | **USB/HID** | Peripheral | `usb` + `esp_hid` | USB HID (S3 USB-OTG) and BLE HID (all targets) — keyboards, mice, gamepads |
| 4 | **Camera** | Peripheral | `esp32-camera` (managed) | Camera interface (ESP32, S3 — C3 has no camera support) |
| 5 | **GPIO Expander** | Peripheral | `esp_io_expander` (managed) | I2C/SPI-based port expanders |
| 6 | **Analog Comparator** | Peripheral | `esp_driver_ana_cmpr` | Analog signal comparison (C3, S3) |
| 7 | **ULP** | Power | `ulp` | Ultra Low Power coprocessor programming (ESP32, S3 only) |
| 8 | **eFuse** | System | `efuse` | One-time programmable memory reading |
| 9 | **Local Control** | System | `esp_local_ctrl` | Local device control over BLE/WiFi |

**Theme:** Mesh networking, USB peripherals, cameras, ultra-low-power compute, and wired connectivity — advanced capabilities that differentiate products.

---

## Intentionally Excluded

Too niche, internal, or out of HAL scope:

| Module | Reason |
|--------|--------|
| HTTPS Server | Not a separate module — will be a TLS config option on the existing `http_server` module |
| OpenThread | Requires IEEE 802.15.4 radio (ESP32-H2/C6 only) — not available on ESP32/C3/S3 targets |
| CoAP | MQTT covers IoT messaging; CoAP is niche/academic; not needed for Thread/Matter |
| Event Loop | ESP-IDF internal pub/sub — blusys modules already abstract events via callbacks |
| SmartConfig | Legacy provisioning — WiFi Provisioning (BLE/SoftAP) is the modern replacement |
| Secure Boot | Bootloader-level build/flash config (`CONFIG_SECURE_BOOT`), not a runtime API |
| Encrypted OTA | Build config + partition table setup, not a runtime API |
| ESP-TLS | Internal plumbing — already used by HTTP, MQTT, HTTPS modules transparently |
| Hardware Crypto | Transparent — mbedTLS automatically uses hardware acceleration |
| Partition Manager | Raw flash access covered by FatFS/SD SPI/NVS; too low-level for HAL |
| USB Serial JTAG | Debugging-only, not an app-facing peripheral |
| Parallel I/O | S3-only, very niche |
| Touch Element | `touch` module already exists; higher-level abstraction adds little |
| App Trace | JTAG profiling tool, not a peripheral |
| JPEG Codec | Very specialized (S3 only) |
| ISP | Camera pipeline specific |
| Bit Scrambler | Niche security feature |
| PPA | Graphics-specific (S3) |
| SDIO | Rare use case |
| Protocomm | Internal to provisioning |
| IEEE 802.15.4 raw | Requires ESP32-H2/C6 radio hardware — not available on ESP32/C3/S3 targets |
| TinyUSB | Overlaps USB Host/Device |
| Key Manager | Very niche security use case |
| RTC Time | Covered by Timer + Sleep modules |
| Ring Buffer | FreeRTOS utility — too low-level for HAL |
| DMA Utilities | Internal memory allocation — too low-level |
| Core Dump | Debugging tool, not a peripheral |
| NVS Encryption | Niche security extension of existing NVS |
| Performance Monitor | Debugging/profiling tool |
| SPI Flash Direct | Covered by NVS/FS/Partition Manager |

---

## Coverage Summary

| Version | Modules | Cumulative | Typical ESP32 Project Coverage |
|---------|---------|------------|-------------------------------|
| V1 | 8 | 8 | ~30% — core peripherals |
| V2 | 16 | 24 | ~50% — extended peripherals |
| V3 | 12 | 36 | ~70% — connectivity & system services |
| **V4** | 9 | **45** | **~84% — production-ready IoT** |
| **V5** | 9 | **54** | **~95% — comprehensive IoT framework** |

**Split logic:** V4 is what you *need* to ship. V5 is what you *want* to differentiate.

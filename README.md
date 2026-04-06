# Blusys HAL

Blusys HAL is a simplified C hardware abstraction layer for ESP32 devices on top of ESP-IDF v5.5.

It provides a smaller API than raw ESP-IDF for common embedded tasks while keeping one shared public surface across supported targets.

## Supported Targets

- `esp32`
- `esp32c3`
- `esp32s3`

## Included Modules

- foundation: `version`, `error`, `target`
- system: `system`
- digital IO: `gpio`
- communication: `uart`, `i2c`, `i2c_slave`, `spi`, `spi_slave`, `twai`
- audio: `i2s` (TX and RX)
- timing and analog: `pwm`, `adc`, `timer`, `pcnt`, `rmt` (TX and RX), `touch`, `dac`, `mcpwm`, `sdm`
- storage: `sdmmc`
- sensors: `temp_sensor`
- system services: `wdt`, `sleep`
- connectivity: `wifi`, `http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`, `espnow`
- bluetooth: `bluetooth`, `ble_gatt`
- storage: `fs`, `nvs`

Target notes:
- `pcnt` is available on `esp32` and `esp32s3`; `esp32c3` reports it as unsupported
- `touch` is available on `esp32` and `esp32s3`; `esp32c3` reports it as unsupported
- `dac` is available on `esp32`; `esp32c3` and `esp32s3` report it as unsupported
- all other modules are available on all three targets

Async support includes:
- GPIO interrupt callbacks
- UART async TX and RX callbacks
- timer callbacks
- PCNT watch point callbacks
- TWAI RX callbacks

## Quick Start

Install blusys (once):

```sh
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
echo 'export PATH="$HOME/.blusys:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Create and build a project:

```sh
mkdir ~/my_project && cd ~/my_project
blusys create
blusys build esp32s3
```

Flash and monitor:

```sh
blusys run /dev/ttyACM0 esp32s3
```

Check your setup:

```sh
blusys version
```

The build target must match the connected board. ESP-IDF v5.5+ is required and auto-detected.

## Documentation

- Docs site: `https://oguzkaganozt.github.io/blusys/`
- Changelog: `CHANGELOG.md`
- Overview: `docs/index.md`
- Getting started: `docs/guides/getting-started.md`
- Examples and task guides: `docs/guides/`
- API reference: `docs/modules/`
- Contributor guidance: `docs/contributing.md`

## Examples

- `examples/smoke/`
- `examples/system_info/`
- `examples/gpio_basic/`
- `examples/gpio_interrupt/`
- `examples/uart_loopback/`
- `examples/uart_async/`
- `examples/i2c_scan/`
- `examples/i2s_basic/`
- `examples/spi_loopback/`
- `examples/pwm_basic/`
- `examples/adc_basic/`
- `examples/dac_basic/`
- `examples/timer_basic/`
- `examples/pcnt_basic/`
- `examples/rmt_basic/`
- `examples/twai_basic/`
- `examples/touch_basic/`
- `examples/concurrency_i2c/`
- `examples/concurrency_spi/`
- `examples/concurrency_timer/`
- `examples/concurrency_uart/`
- `examples/sdmmc_basic/`
- `examples/temp_sensor_basic/`
- `examples/wdt_basic/`
- `examples/sleep_basic/`
- `examples/mcpwm_basic/`
- `examples/sdm_basic/`
- `examples/i2c_slave_basic/`
- `examples/spi_slave_basic/`
- `examples/i2s_rx_basic/`
- `examples/rmt_rx_basic/`
- `examples/wifi_basic/`
- `examples/nvs_basic/`
- `examples/http_client_basic/`
- `examples/http_server_basic/`
- `examples/mqtt_basic/`
- `examples/ota_basic/`
- `examples/sntp_basic/`
- `examples/mdns_basic/`
- `examples/bluetooth_basic/`
- `examples/fs_basic/`
- `examples/espnow_basic/`
- `examples/ble_gatt_basic/`

## Project Status

`v3.0.0` is released.
Full HAL peripheral coverage and connectivity stack are complete: 35 modules covering core peripherals, symmetric counterparts, system services, WiFi/HTTP/MQTT networking, Bluetooth/BLE, and storage.
`V4` (advanced peripherals and ecosystem helpers) is planned: `eth`, `usb`, `lcd`, `efuse`, `ulp`, provisioning, and more.

Detailed implementation tracking remains in `PROGRESS.md`.

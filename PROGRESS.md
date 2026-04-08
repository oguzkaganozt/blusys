# Progress

## Current Summary

- current phase: `V6`
- current release: `v5.0.0`
- overall status: `not_started`
- open blockers: none

## Roadmap

| Version | Status | Modules |
|---------|--------|---------|
| V1 | completed | `system`, `gpio`, `uart`, `i2c`, `spi`, `pwm`, `adc`, `timer` |
| V2 | completed | `pcnt`, `rmt`, `twai`, `i2s`, `touch`, `dac`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`, `mcpwm`, `sdm`, `i2c_slave`, `spi_slave`, `i2s_rx`, `rmt_rx` |
| V3 | completed | `wifi`, `nvs`, `http_client`, `mqtt`, `http_server`, `ota`, `sntp`, `mdns`, `bluetooth`, `fs`, `espnow`, `ble_gatt` |
| V4 | completed | `button`, `led_strip`, `console`, `fatfs`, `sd_spi`, `power_mgmt`, `ws_client`, `wifi_prov`, `lcd`, standalone CLI |
| V5 | completed | `encoder`, `buzzer`, `one_wire`, `dht`, `gpio_expander`, `efuse`, `ulp`, `usb_host`, `usb_device`, `usb_hid`, `wifi_mesh`, `local_ctrl`, services restructure |
| V6 | not_started | `ethernet`, `camera` |

**Deferred:** `ana_cmpr` — requires `SOC_ANA_CMPR_SUPPORTED`; not available on ESP32, ESP32-C3, or ESP32-S3.

## Validation Snapshot

- v1–v5 release validation completed
- `mkdocs build --strict` passes
- pending hardware smoke tests: `seven_seg`, `dht`, `usb_host`, `usb_device`, `usb_hid`, `efuse`, `ulp`, `local_ctrl`, `wifi_mesh`

## Next Actions

1. Hardware smoke test `dht` (DHT11, DHT22)
2. Hardware smoke test `gpio_expander` (PCF8574, MCP23017)
3. Hardware smoke test `efuse` (identity and security-state spot-check)
4. Hardware smoke test `ulp` (ESP32, ESP32-S3)
5. Hardware smoke test `wifi_mesh` (two-node mesh)

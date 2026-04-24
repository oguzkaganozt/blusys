# Examples

The example tree is intentionally small. Start with the reference examples, use validation examples for regression only.

## Learn first

- **connectivity** - Wi-Fi, HTTP, and MQTT in a compact connected product
- **display** - LCD, LVGL, encoder, and UI flow
- **hal** - GPIO, UART, I2C, SPI, ADC, PWM, timer, NVS, and more

## Learn next

- **atlas** - full connected product composition and provisioning
- **override** - product-local capability and explicit app spec

## Validation only

- **connected_headless** / **connected_device** - legacy regression coverage
- **headless_telemetry_low_power** - policy and low-power smoke
- **framework_device_basic** and the rest of `validation/` - CI and hardware smokes

## Notes

- Inventory and example selection live in `inventory.yml`.
- `docs/start/` is the recommended new-product path.

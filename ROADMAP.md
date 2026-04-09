# Roadmap

Blusys V1 (v6.x) is complete. This document tracks planned V2 work and
near-term V1.1 follow-ups.

## V1.1 follow-ups

These items do not block the current release but are tracked so they are not lost.

### End-to-end encoder + LCD rig

Each piece has been individually validated on real hardware, but no single session has
wired a physical EC11 encoder, an ST7735 panel, and the framework spine into one rig and
demonstrated encoder-rotation → focus-traversal → confirm-press → overlay-show
end-to-end.

Recommended setup: esp32-s3 + ST7735 SPI + EC11 encoder, using the interactive scaffold
`app_main.cpp` LCD init TODO block as the reference.

### Scope-based input → feedback latency measurement

Wire `intent::confirm` emission to a debug GPIO toggle inside `controller::handle`. Tap
that GPIO + the first `blusys_lcd_draw_bitmap` SPI CS edge with a Saleae or oscilloscope.
Record median + p99 over a few hundred presses. File the report under `docs/guides/`
following the hardware-validation report naming convention
(`docs/guides/hardware-validation-report-template.md`).

### SSD1306 bus recovery on `blusys_lcd_open`

A previous failed boot can leave an I2C slave (SSD1306 in particular) holding SDA low
mid-transaction, locking the bus. Standard recovery sequence before `i2c_new_master_bus`:
take SDA/SCL as GPIO; if SDA reads low, pulse SCL 9 times to flush the stuck byte, issue
a manual STOP condition (SDA low → SCL high → SDA high), then release the GPIOs back to
the I2C peripheral.

Blocked on a deliberate stuck-state reproduction so the fix can be validated. ~20-line
isolated change once the reproduction is in hand.

### `widget_kit_demo` keyboard encoder simulation

Map arrow keys to `LV_INDEV_TYPE_ENCODER` events in `scripts/host/src/widget_kit_demo.cpp`
so encoder focus traversal can be validated visually on host without real hardware.
Currently only `framework_encoder_basic` does this on-device.

---

## V2 scope

V2 items are not yet scheduled. They are captured here to preserve the reasoning from
the V1 transition.

### `bu_knob` — Camp 3 rotary widget

A dedicated knob widget using `lv_obj_class_t` embedded storage (Camp 3) instead of the
external slot pool (Camp 2). The implementation template differs from the V1 Camp 2
widgets and was deferred to V2 so the Camp 2 pattern could stabilize across five widgets
before introducing the second pattern.

### Services → C++ migration

`blusys_services` is currently C (opaque handles, explicit lifecycle). Migrating it to
C++ would let service callbacks use captureless lambdas and align the services tier
with the framework tier's language. Deferred because:
- the existing C implementations (`wifi.c`, `mqtt.c`, `ota.c`) are already
  well-factored and clean
- migrating in V1 would force every service example from `.c` to `.cpp`
- no concrete pain point has shown up yet that C cannot address

Migration should be driven by a real pain point, not as a general cleanup.

### Per-module build gating

V1 uses a binary `BLUSYS_STARTER_TYPE` (`headless` or `interactive`) to gate the UI
tier. V2 could introduce per-module build flags (`BLUSYS_MODULE_WIFI`, `BLUSYS_MODULE_BLE`,
etc.) to let products strip unused services from the binary. Worth revisiting once there
are three or more products with different module subsets.

### Latency tooling

On-device input → feedback latency tooling (GPIO-toggle + scope or logic analyser
integration) as a standard part of the validation chain. Currently measured ad-hoc;
V2 could formalize the methodology and add a template report.

### LVGL version track

LVGL v9.2.2 is pinned in `scripts/host/CMakeLists.txt` and in each interactive example's
`idf_component.yml`. Review the pin when LVGL v9.3+ or v10.x stabilize. The main risks
are API surface changes in `lv_arc`, `lv_switch`, and the `LV_EVENT_*` set that the
widget kit directly uses.

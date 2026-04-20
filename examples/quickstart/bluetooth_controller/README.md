# Bluetooth volume controller (quickstart)

Forkable **BLE HID** reference: three buttons on a headless device pair with any laptop as a standards-compliant HID media remote and control the host OS volume natively — no companion app.

## Locked brief

| Area | Choice |
|------|--------|
| **Purpose** | Real HOGP peripheral (HID over GATT) advertising as a consumer-control remote. Windows, macOS, and Linux recognize it out of the box. |
| **Theme** | Display-less — status LED only (off / blink while advertising / solid once paired). |
| **Inputs** | 3 GPIO buttons: Vol+, Vol−, Mute. Wired via `blusys/drivers/button.h` with pull-up, active-low, 50 ms debounce. Host build cycles through the buttons on a timer so the reducer is exercised without a peripheral. |
| **Core screens** | None. |
| **Capabilities** | `ble_hid_device` (framework — `components/blusys/.../capabilities/ble_hid_device.hpp`) |
| **Productization bar** | Native OS pairing, bonded reconnect (keys persist in NVS), battery level reported, advertising resumes on disconnect. |

## Layout

- `main/core/` — `state`, `action`, `update()`; reducer stays free of radio calls (talks to HID through an injected function pointer).
- `main/platform/` — capability instantiation, button wiring, LED wiring, `on_init` hooks, `BLUSYS_APP_HEADLESS` entry.
- `main/Kconfig.projbuild` — GPIO assignments and advertised device name.

## Run

From the **Blusys repo root**:

```bash
blusys host-build examples/quickstart/bluetooth_controller
# → examples/quickstart/bluetooth_controller/build-host/bluetooth_controller_host

blusys build examples/quickstart/bluetooth_controller esp32c3
```

## What you see on host

The host binary logs `[ble_hid_device] open 'Blusys Controller' (host stub)`, then after ~1 s a simulated client-connect, then it cycles through Vol+ / Vol− / Mute on a 3-second cadence. Each press prints the emitted consumer usage:

```
[ble_hid_device] consumer usage=0x00e9 pressed=1 state=0x01
[ble_hid_device] consumer usage=0x00e9 pressed=0 state=0x00
```

The reducer logs `phase=advertising → connected` and a running `vol_est` / `muted` summary.

## Pair with a laptop (device build)

1. Flash the firmware. The status LED starts blinking — the device is advertising as **Blusys Controller**.
2. On macOS: *System Settings → Bluetooth*. On Linux: `bluetoothctl` → `scan on` → `pair <addr>` → `trust`. On Windows: *Settings → Bluetooth & devices → Add device*.
3. After pairing the LED goes solid. Press Vol+/Vol−/Mute on the controller — the OS volume responds.
4. Power-cycle the controller and it reconnects automatically via stored bonding keys.

## Bringing up without real buttons

`BTCTRL_SIMULATE_ON_CONNECT` (Kconfig, default **on**) makes the firmware auto-emit 3× Vol+ followed by 3× Vol− every time a client connects. Handy for bench-testing when no buttons are wired. Turn it off (`blusys config`, or set `# CONFIG_BTCTRL_SIMULATE_ON_CONNECT is not set` in `sdkconfig.defaults`) once real inputs are attached.

## What to edit first

1. `main/Kconfig.projbuild` — swap GPIO pins and device name to match your hardware, disable the auto-simulation.
2. `main/core/app_logic.cpp` — add more intents (e.g. Play/Pause is already in the HID report map — bit 3). Extend `enum class intent` and the switch in `update()`.
3. `main/platform/app_main.cpp` — if you need more buttons or a rotary encoder, add more `button_slot`s or swap in `blusys/drivers/encoder.h`.

## Extending the report map

The HID report map (see `components/blusys/src/services/net/ble_hid_device.c`, `kReportMap`) currently declares four consumer-control bits. To add media transport (Next/Prev) or keyboard reports, extend `kReportMap` and either widen the consumer byte or add a second report ID. See `docs/services/ble_hid_device.md` for the full service surface.

## Coexistence

`ble_hid_device_capability` acquires the BLE controller through `blusys_bt_stack`. It cannot run concurrently with `bluetooth_capability`, `ble_gatt`, BLE-based Wi-Fi provisioning, or `usb_hid` in BLE-central mode — the second opener receives `BLUSYS_ERR_BUSY`.

# USB Support Implementation Plan for Blusys

## Context

Blusys currently has no USB module. PROGRESS.md (line 76) lists `usb_hid` as a planned V5 module. This plan adds comprehensive USB support across three modules:

1. **`usb_host`** — HAL module: USB host stack lifecycle and device enumeration (ESP32-S3 only)
2. **`usb_device`** — HAL module: USB device mode via TinyUSB (ESP32-S3 only)
3. **`usb_hid`** — Service module: HID input via USB OTG (S3) and BLE HID (all targets)

**Hardware reality:**

| Target    | USB OTG (Host/Device) | USB Serial/JTAG |
|-----------|-----------------------|-----------------|
| ESP32     | No                    | No              |
| ESP32-C3  | No                    | Yes             |
| ESP32-S3  | Yes (DWC2)           | Yes             |

**Constraint:** ESP32-S3 USB OTG operates in either host OR device mode, not both simultaneously. The two HAL modules are mutually exclusive at runtime.

---

## Phase 1: Feature Flags & Capability Registration

### 1.1 Feature Enum — `components/blusys/include/blusys/target.h`

Add before `BLUSYS_FEATURE_COUNT` (line 68):

```c
BLUSYS_FEATURE_USB_HOST,
BLUSYS_FEATURE_USB_DEVICE,
BLUSYS_FEATURE_USB_HID,
```

### 1.2 Feature Masks — `components/blusys/include/blusys/internal/blusys_target_caps.h`

Add feature mask defines (after line 87):

```c
#define BLUSYS_USB_HOST_FEATURE_MASK    BLUSYS_FEATURE_MASK(BLUSYS_FEATURE_USB_HOST)
#define BLUSYS_USB_DEVICE_FEATURE_MASK  BLUSYS_FEATURE_MASK(BLUSYS_FEATURE_USB_DEVICE)
#define BLUSYS_USB_HID_FEATURE_MASK     BLUSYS_FEATURE_MASK(BLUSYS_FEATURE_USB_HID)
```

Add `BLUSYS_USB_HID_FEATURE_MASK` to `BLUSYS_BASE_FEATURE_MASK` (BLE HID works on all targets).
Do NOT add `USB_HOST` or `USB_DEVICE` to base mask (S3-only).

### 1.3 Per-Target Capabilities

**`src/targets/esp32s3/target_caps.c`** — add `BLUSYS_USB_HOST_FEATURE_MASK | BLUSYS_USB_DEVICE_FEATURE_MASK` to `.feature_mask`

**`src/targets/esp32/target_caps.c`** and **`src/targets/esp32c3/target_caps.c`** — no changes needed (`USB_HID` comes via base mask)

---

## Phase 2: USB Host HAL Module

### 2.1 Public Header — `components/blusys/include/blusys/usb_host.h`

```c
#ifndef BLUSYS_USB_HOST_H
#define BLUSYS_USB_HOST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_usb_host blusys_usb_host_t;

typedef struct {
    uint16_t vid;
    uint16_t pid;
    uint8_t  dev_addr;
} blusys_usb_host_device_info_t;

typedef enum {
    BLUSYS_USB_HOST_EVENT_DEVICE_CONNECTED,
    BLUSYS_USB_HOST_EVENT_DEVICE_DISCONNECTED,
} blusys_usb_host_event_t;

typedef void (*blusys_usb_host_event_callback_t)(
    blusys_usb_host_t *host,
    blusys_usb_host_event_t event,
    const blusys_usb_host_device_info_t *device,
    void *user_ctx);

typedef struct {
    blusys_usb_host_event_callback_t event_cb;
    void *event_user_ctx;
} blusys_usb_host_config_t;

blusys_err_t blusys_usb_host_open(const blusys_usb_host_config_t *config,
                                   blusys_usb_host_t **out_host);
blusys_err_t blusys_usb_host_close(blusys_usb_host_t *host);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_HOST_H */
```

### 2.2 Implementation — `components/blusys/src/common/usb_host.c`

- **SOC guard:** `#if SOC_USB_OTG_SUPPORTED` with stubs on ESP32/ESP32-C3
- **Singleton pattern** (like `ble_gatt`): global `s_usb_host_handle`, `ensure_global_lock()` helper
- **Two FreeRTOS tasks:**
  - **Daemon task:** calls `usb_host_lib_handle_events()` in a loop — manages bus-level events
  - **Client task:** calls `usb_host_client_handle_events()` in a loop — manages client-level events
- **Device connect flow:** open device handle → read device descriptor → populate `blusys_usb_host_device_info_t` → invoke user callback → close device handle (class drivers will re-open as needed)
- **Close flow:** signal tasks to exit, wait for clean shutdown, `usb_host_client_deregister()`, `usb_host_uninstall()`
- **Thread safety:** `blusys_lock_t` + singleton guard with `portMUX_TYPE`
- **Critical rule:** never hold the lock across blocking waits (consistent with the blusys lock rule)

### 2.3 Build Registration — `components/blusys/CMakeLists.txt`

- Add `src/common/usb_host.c` to `blusys_sources` list
- Add `usb` to the `REQUIRES` list (this is ESP-IDF's built-in USB host component providing `usb/usb_host.h`)

### 2.4 Umbrella Header — `components/blusys/include/blusys/blusys.h`

Add `#include "blusys/usb_host.h"`

---

## Phase 3: USB Device HAL Module

### 3.1 Public Header — `components/blusys/include/blusys/usb_device.h`

```c
#ifndef BLUSYS_USB_DEVICE_H
#define BLUSYS_USB_DEVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_usb_device blusys_usb_device_t;

typedef enum {
    BLUSYS_USB_DEVICE_CLASS_CDC,
    BLUSYS_USB_DEVICE_CLASS_HID,
    BLUSYS_USB_DEVICE_CLASS_MSC,
} blusys_usb_device_class_t;

typedef enum {
    BLUSYS_USB_DEVICE_EVENT_CONNECTED,
    BLUSYS_USB_DEVICE_EVENT_DISCONNECTED,
    BLUSYS_USB_DEVICE_EVENT_SUSPENDED,
    BLUSYS_USB_DEVICE_EVENT_RESUMED,
} blusys_usb_device_event_t;

typedef void (*blusys_usb_device_event_callback_t)(
    blusys_usb_device_t *dev,
    blusys_usb_device_event_t event,
    void *user_ctx);

typedef void (*blusys_usb_device_cdc_rx_callback_t)(
    blusys_usb_device_t *dev,
    const uint8_t *data,
    size_t len,
    void *user_ctx);

typedef struct {
    uint16_t    vid;
    uint16_t    pid;
    const char *manufacturer;
    const char *product;
    const char *serial;
    blusys_usb_device_class_t device_class;
    blusys_usb_device_event_callback_t event_cb;
    void *event_user_ctx;
} blusys_usb_device_config_t;

blusys_err_t blusys_usb_device_open(const blusys_usb_device_config_t *config,
                                     blusys_usb_device_t **out_dev);
blusys_err_t blusys_usb_device_close(blusys_usb_device_t *dev);

/* CDC class operations */
blusys_err_t blusys_usb_device_cdc_write(blusys_usb_device_t *dev,
                                          const void *data, size_t len);
blusys_err_t blusys_usb_device_cdc_set_rx_callback(
    blusys_usb_device_t *dev,
    blusys_usb_device_cdc_rx_callback_t callback,
    void *user_ctx);

/* HID class operations */
blusys_err_t blusys_usb_device_hid_send_report(blusys_usb_device_t *dev,
                                                 const uint8_t *report,
                                                 size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_DEVICE_H */
```

### 3.2 Implementation — `components/blusys/src/common/usb_device.c`

- **SOC guard:** `#if SOC_USB_OTG_SUPPORTED` with stubs on ESP32/ESP32-C3
- **Managed component dependency:** Requires `espressif/esp_tinyusb`, detected at build time via `BLUSYS_HAS_TINYUSB`
- If `BLUSYS_HAS_TINYUSB` is not defined, all functions return `BLUSYS_ERR_NOT_SUPPORTED`
- **Singleton pattern:** only one USB device instance at a time
- **TinyUSB wrapping:**
  - `open` → `tinyusb_driver_install()` with class-specific descriptor configuration
  - `close` → `tinyusb_driver_uninstall()`
  - CDC path: `tinyusb_cdcacm_register()` / `tinyusb_cdcacm_write_queue()`
  - HID path: `tud_hid_report()` via TinyUSB HID callbacks
  - MSC path: `tud_msc_*` callbacks for block device access (future extension)
- **Thread safety:** `blusys_lock_t` for all operations

### 3.3 Build Registration — `components/blusys/CMakeLists.txt`

Add `src/common/usb_device.c` to `blusys_sources`.

Add managed component detection block (after `idf_component_register`):

```cmake
# Optional managed component: espressif/esp_tinyusb
idf_build_get_property(build_components BUILD_COMPONENTS)
if("esp_tinyusb" IN_LIST build_components OR "espressif__esp_tinyusb" IN_LIST build_components)
    idf_component_get_property(tinyusb_lib esp_tinyusb COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${tinyusb_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_TINYUSB=1)
endif()
```

### 3.4 Umbrella Header — `components/blusys/include/blusys/blusys.h`

Add `#include "blusys/usb_device.h"`

---

## Phase 4: USB HID Service Module

### 4.1 Public Header — `components/blusys_services/include/blusys/input/usb_hid.h`

```c
#ifndef BLUSYS_USB_HID_H
#define BLUSYS_USB_HID_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct blusys_usb_hid blusys_usb_hid_t;

typedef enum {
    BLUSYS_USB_HID_TRANSPORT_USB,   /* ESP32-S3 only (USB OTG host) */
    BLUSYS_USB_HID_TRANSPORT_BLE,   /* All targets (NimBLE HOGP client) */
} blusys_usb_hid_transport_t;

typedef struct {
    uint8_t modifier;
    uint8_t keycodes[6];
} blusys_usb_hid_keyboard_report_t;

typedef struct {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
    int8_t  wheel;
} blusys_usb_hid_mouse_report_t;

typedef struct {
    const uint8_t *data;
    size_t         len;
} blusys_usb_hid_raw_report_t;

typedef enum {
    BLUSYS_USB_HID_EVENT_CONNECTED,
    BLUSYS_USB_HID_EVENT_DISCONNECTED,
    BLUSYS_USB_HID_EVENT_KEYBOARD_REPORT,
    BLUSYS_USB_HID_EVENT_MOUSE_REPORT,
    BLUSYS_USB_HID_EVENT_RAW_REPORT,
} blusys_usb_hid_event_t;

typedef struct {
    blusys_usb_hid_event_t event;
    union {
        blusys_usb_hid_keyboard_report_t keyboard;
        blusys_usb_hid_mouse_report_t    mouse;
        blusys_usb_hid_raw_report_t      raw;
    } data;
} blusys_usb_hid_event_data_t;

typedef void (*blusys_usb_hid_callback_t)(
    blusys_usb_hid_t *hid,
    const blusys_usb_hid_event_data_t *event,
    void *user_ctx);

typedef struct {
    blusys_usb_hid_transport_t transport;
    blusys_usb_hid_callback_t  callback;
    void                      *user_ctx;
    const char                *ble_device_name; /* BLE only: target peripheral name filter */
} blusys_usb_hid_config_t;

blusys_err_t blusys_usb_hid_open(const blusys_usb_hid_config_t *config,
                                  blusys_usb_hid_t **out_hid);
blusys_err_t blusys_usb_hid_close(blusys_usb_hid_t *hid);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_USB_HID_H */
```

### 4.2 Implementation — `components/blusys_services/src/input/usb_hid.c`

Dual-path architecture with compile-time transport availability:

**USB transport path** (`#if SOC_USB_OTG_SUPPORTED && defined(BLUSYS_HAS_USB_HOST_HID)`):
- Uses `espressif/usb_host_hid` managed component for HID class driver
- Internally calls `blusys_usb_host_open()` to set up the USB host stack
- Registers HID class driver input report callbacks
- Parses boot-protocol keyboard/mouse reports into `blusys_usb_hid_*_report_t` structs
- Fires `BLUSYS_USB_HID_EVENT_RAW_REPORT` for gamepads and other HID device types

**BLE transport path** (`#if defined(CONFIG_BT_NIMBLE_ENABLED)`):
- Implements HID over GATT Profile (HOGP) client via NimBLE
- BLE scan for HID peripherals → connect → discover HID service → subscribe to report characteristics
- Boot-protocol keyboard/mouse initially; report-protocol parsing can be extended later
- Requires `blusys_nvs_ensure_init()` for BLE stack (consistent with `ble_gatt` module)

**Public API dispatch in `blusys_usb_hid_open()`:**
- Checks `config->transport` and routes to the compiled-in transport path
- Returns `BLUSYS_ERR_NOT_SUPPORTED` if the requested transport is not available at compile time

### 4.3 Build Registration — `components/blusys_services/CMakeLists.txt`

Add `src/input/usb_hid.c` to `blusys_services_sources` (under `# input` section).

Add managed component detection block (after existing mdns/lvgl blocks):

```cmake
# Optional managed component: espressif/usb_host_hid
if("usb_host_hid" IN_LIST build_components OR "espressif__usb_host_hid" IN_LIST build_components)
    idf_component_get_property(usb_host_hid_lib usb_host_hid COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${usb_host_hid_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_USB_HOST_HID=1)
endif()
```

### 4.4 Umbrella Header — `components/blusys_services/include/blusys/blusys_services.h`

Add `#include "blusys/input/usb_hid.h"` under the input category section.

---

## Phase 5: Examples

### 5.1 `examples/usb_host_basic/`

Demonstrates USB host stack: opens host, logs device connect/disconnect events with VID/PID.

| File | Notes |
|------|-------|
| `CMakeLists.txt` | Standard root, `EXTRA_COMPONENT_DIRS ../../components` |
| `main/CMakeLists.txt` | `REQUIRES blusys` |
| `main/usb_host_basic_main.c` | Open host → log events in callback → close on Ctrl+C |
| `sdkconfig.esp32s3` | USB host config defaults |

No managed component dependency — `usb` is built into ESP-IDF.

### 5.2 `examples/usb_device_basic/`

Demonstrates USB device mode: configures ESP32-S3 as USB CDC serial device, echoes data from host PC.

| File | Notes |
|------|-------|
| `CMakeLists.txt` | Standard root |
| `main/CMakeLists.txt` | `REQUIRES blusys` |
| `main/usb_device_basic_main.c` | Open CDC device → echo received data → close |
| `main/idf_component.yml` | Declares `espressif/esp_tinyusb` dependency |
| `sdkconfig.esp32s3` | TinyUSB config defaults |

### 5.3 `examples/usb_hid_basic/`

Demonstrates HID input: receives keyboard/mouse reports via USB or BLE transport.

| File | Notes |
|------|-------|
| `CMakeLists.txt` | Standard root |
| `main/CMakeLists.txt` | `REQUIRES blusys_services` |
| `main/usb_hid_basic_main.c` | Open HID → log reports in callback |
| `main/idf_component.yml` | Declares `espressif/usb_host_hid` dependency |
| `main/Kconfig.projbuild` | Transport selector: USB vs BLE |
| `sdkconfig.esp32s3` | USB path defaults |

---

## Phase 6: Documentation

### New Documentation Files

| File | Type | Description |
|------|------|-------------|
| `docs/modules/usb_host.md` | API Reference | Types, Functions, Lifecycle, Thread Safety, Limitations |
| `docs/modules/usb_device.md` | API Reference | Types, Functions, Lifecycle, Thread Safety, Limitations |
| `docs/modules/usb_hid.md` | API Reference | Types, Functions, Lifecycle, Thread Safety, Limitations |
| `docs/guides/usb-host-basic.md` | Task Guide | Problem → Prerequisites → Minimal Example → Common Mistakes |
| `docs/guides/usb-device-basic.md` | Task Guide | Problem → Prerequisites → Minimal Example → Common Mistakes |
| `docs/guides/usb-hid-basic.md` | Task Guide | Problem → Prerequisites → Minimal Example → Common Mistakes |

### Navigation Updates

**`mkdocs.yml`** — add entries under:
- Guides > HAL > Device: `usb-host-basic.md`, `usb-device-basic.md`
- Guides > Services > Input: `usb-hid-basic.md`
- API Reference > HAL > Device: `usb_host.md`, `usb_device.md`
- API Reference > Services > Input: `usb_hid.md`

**`docs/modules/index.md`** — add USB entries to the Device card grid (HAL) and Input card grid (Services)

**`docs/guides/index.md`** — add USB entries to the corresponding card grids

---

## Phase 7: PROGRESS.md Updates

- Add `usb_host`, `usb_device` to V5 done list
- Move `usb_hid` from planned to done
- Update HAL Public API section with `blusys_usb_host_*`, `blusys_usb_device_*` prefixes
- Update Services Public API input section with `blusys_usb_hid_*` prefix
- Add all three modules to Recent Work
- Add to Validation Snapshot

---

## Complete File Manifest

### New Files (15+)

| File | Description |
|------|-------------|
| `components/blusys/include/blusys/usb_host.h` | USB host HAL public header |
| `components/blusys/src/common/usb_host.c` | USB host HAL implementation |
| `components/blusys/include/blusys/usb_device.h` | USB device HAL public header |
| `components/blusys/src/common/usb_device.c` | USB device HAL implementation |
| `components/blusys_services/include/blusys/input/usb_hid.h` | USB HID service public header |
| `components/blusys_services/src/input/usb_hid.c` | USB HID service implementation |
| `examples/usb_host_basic/` (4 files) | USB host example project |
| `examples/usb_device_basic/` (5 files) | USB device example project |
| `examples/usb_hid_basic/` (6 files) | USB HID example project |
| `docs/modules/usb_host.md` | USB host API reference |
| `docs/modules/usb_device.md` | USB device API reference |
| `docs/modules/usb_hid.md` | USB HID API reference |
| `docs/guides/usb-host-basic.md` | USB host task guide |
| `docs/guides/usb-device-basic.md` | USB device task guide |
| `docs/guides/usb-hid-basic.md` | USB HID task guide |

### Modified Files (11)

| File | Change |
|------|--------|
| `components/blusys/include/blusys/target.h` | Add 3 feature enum entries before `BLUSYS_FEATURE_COUNT` |
| `components/blusys/include/blusys/internal/blusys_target_caps.h` | Add 3 mask defines; add `USB_HID` to base mask |
| `components/blusys/src/targets/esp32s3/target_caps.c` | Add `USB_HOST` + `USB_DEVICE` feature masks |
| `components/blusys/CMakeLists.txt` | Add 2 source files, `usb` to REQUIRES, TinyUSB detection block |
| `components/blusys_services/CMakeLists.txt` | Add `usb_hid.c` to sources, `usb_host_hid` detection block |
| `components/blusys/include/blusys/blusys.h` | Add 2 `#include` lines |
| `components/blusys_services/include/blusys/blusys_services.h` | Add 1 `#include` line |
| `mkdocs.yml` | Add 6 navigation entries |
| `docs/modules/index.md` | Add USB entries to card grids |
| `docs/guides/index.md` | Add USB entries to card grids |
| `PROGRESS.md` | Update V5 status, API surface, recent work |

---

## Implementation Order

```
Phase 1  Feature flags & capability registration       (all 3 modules)
Phase 2  USB Host HAL module                            (foundation for USB HID)
Phase 3  USB Device HAL module                          (independent of Phase 2)
Phase 4  USB HID Service module                         (depends on Phase 2 for USB transport)
Phase 5  Examples                                       (one per module)
Phase 6  Documentation                                  (API refs + task guides)
Phase 7  PROGRESS.md updates
```

Phases 2 and 3 are independent and can be done in parallel.
Phase 4 depends on Phase 2 (USB transport path calls `blusys_usb_host_open()`).

---

## Verification Checklist

- [ ] `blusys build examples/usb_host_basic esp32s3` — compiles clean
- [ ] `blusys build examples/usb_host_basic esp32` — compiles with stubs
- [ ] `blusys build examples/usb_host_basic esp32c3` — compiles with stubs
- [ ] `blusys build examples/usb_device_basic esp32s3` — compiles clean
- [ ] `blusys build examples/usb_device_basic esp32` — compiles with stubs
- [ ] `blusys build examples/usb_hid_basic esp32s3` — compiles clean (USB path)
- [ ] `blusys build examples/usb_hid_basic esp32c3` — compiles clean (BLE path)
- [ ] `blusys build-examples` — all existing + new examples pass on all 3 targets
- [ ] `mkdocs build --strict` — documentation gate passes with no errors

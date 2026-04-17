#pragma once

#include "blusys/hal/error.h"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"

#include <cstdint>

namespace blusys {

// Integration event ID ranges (reserved, non-overlapping):
//   connectivity   0x0100 – 0x01FF
//   storage        0x0200 – 0x02FF
//   bluetooth      0x0300 – 0x03FF
//   ota            0x0400 – 0x04FF
//   diagnostics    0x0500 – 0x05FF
//   telemetry      0x0600 – 0x06FF
//   provisioning   0x0700 – 0x07FF
//   mqtt (host SDL) 0x0800 – 0x08FF
//   product custom 0x0900 – 0x09FF (map in `capability_event_map.cpp` or handle via
//                  `app_spec::map_event` as `integration_passthrough` until mapped)
//   lan_control    0x0A00 – 0x0AFF
//   usb            0x0B00 – 0x0BFF
//   ble_hid_device 0x0C00 – 0x0CFF

enum class capability_kind : std::uint8_t {
    connectivity,
    storage,
    bluetooth,
    ota,
    diagnostics,
    telemetry,
    provisioning,
    mqtt_host,    // host (SDL) — libmosquitto
    mqtt,         // device — blusys_mqtt service (ESP-IDF)
    lan_control,
    usb,
    ble_hid_device,
    custom,
};

// Common header for every capability status struct. Concrete capabilities
// inherit via aggregate (C-style) composition so `status.capability_ready`
// is available uniformly without per-capability duplication.
struct capability_status_base {
    bool capability_ready = false;
};

// Threading contract for concrete capabilities:
//
//   * `start()`, `poll()`, and `stop()` all run on the single runtime
//     thread that drives the reducer; they never re-enter each other.
//     Implementations can touch their own state from these methods
//     without locking.
//
//   * `poll()` is called at a cadence the runtime chooses — at minimum
//     once per event drain, otherwise roughly per app tick. It must be
//     non-blocking; long work belongs on a dedicated service/worker
//     thread that hands events back via `post_integration_event()`.
//
//   * `post_integration_event()` is the ONLY cross-thread-safe entry
//     into the runtime. Worker threads, ISRs, and service callbacks
//     must funnel all updates through it. Any mutable capability state
//     they touch is the implementation's responsibility to serialize
//     (atomics or a mutex drained from `poll()` are both fine — see
//     `ble_hid_device_capability` and `mqtt_capability` for examples
//     of each pattern).
class capability_base {
public:
    virtual ~capability_base() = default;

    [[nodiscard]] virtual capability_kind kind() const = 0;

    virtual blusys_err_t start(blusys::runtime &rt) = 0;
    virtual void poll(std::uint32_t now_ms) = 0;
    virtual void stop() = 0;

    // Concrete capabilities must also expose `static constexpr
    // capability_kind kind_value` equal to what `kind()` returns.
    // `app_ctx::get<T>()` uses it to locate a capability by type without
    // a registry switch.

protected:
    // Shared integration-event emit helper. Capabilities set `rt_` in start()
    // (typically `rt_ = &rt;`) and clear it in stop(). Safe to call with a
    // null runtime — events are simply dropped. Replaces per-capability
    // `post_event(ev [, code])` forwarders so the runtime ABI stays in one place.
    void post_integration_event(std::uint32_t event_id,
                                std::uint32_t event_code = 0,
                                const void *payload = nullptr)
    {
        if (rt_ != nullptr) {
            rt_->post_event(blusys::make_integration_event(
                event_id, event_code, payload));
        }
    }

    blusys::runtime *rt_ = nullptr;
};

}  // namespace blusys

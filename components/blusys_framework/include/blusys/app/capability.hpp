#pragma once

#include "blusys/error.h"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"

#include <cstdint>

namespace blusys::app {

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

enum class capability_kind : std::uint8_t {
    connectivity,
    storage,
    bluetooth,
    ota,
    diagnostics,
    telemetry,
    provisioning,
    mqtt_host,
    lan_control,
    usb,
    custom,
};

class capability_base {
public:
    virtual ~capability_base() = default;

    [[nodiscard]] virtual capability_kind kind() const = 0;

    virtual blusys_err_t start(blusys::framework::runtime &rt) = 0;
    virtual void poll(std::uint32_t now_ms) = 0;
    virtual void stop() = 0;

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
            rt_->post_event(blusys::framework::make_integration_event(
                event_id, event_code, payload));
        }
    }

    blusys::framework::runtime *rt_ = nullptr;
};

}  // namespace blusys::app

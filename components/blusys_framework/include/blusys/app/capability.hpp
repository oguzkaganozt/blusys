#pragma once

#include "blusys/error.h"

#include <cstdint>

namespace blusys::framework { class runtime; }

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

enum class capability_kind : std::uint8_t {
    connectivity,
    storage,
    bluetooth,
    ota,
    diagnostics,
    telemetry,
    provisioning,
    mqtt_host,
    custom,
};

class capability_base {
public:
    virtual ~capability_base() = default;

    [[nodiscard]] virtual capability_kind kind() const = 0;

    virtual blusys_err_t start(blusys::framework::runtime &rt) = 0;
    virtual void poll(std::uint32_t now_ms) = 0;
    virtual void stop() = 0;
};

}  // namespace blusys::app

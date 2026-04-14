#pragma once

#include "blusys/framework/containers.hpp"

#include <cstddef>
#include <cstdint>

namespace blusys {

enum class feedback_channel : std::uint8_t {
    visual,
    audio,
    haptic,
};

enum class feedback_pattern : std::uint8_t {
    click,
    focus,
    confirm,
    success,
    warning,
    error,
    notification,
};

struct feedback_event {
    feedback_channel channel = feedback_channel::visual;
    feedback_pattern pattern = feedback_pattern::click;
    std::uint32_t    value = 0;
    const void      *payload = nullptr;
};

class feedback_sink {
public:
    virtual ~feedback_sink() = default;

    virtual bool supports(feedback_channel channel) const = 0;
    virtual void emit(const feedback_event &event) = 0;
};

class feedback_bus {
public:
    static constexpr std::size_t kMaxSinks = 6;

    bool register_sink(feedback_sink *sink);
    bool unregister_sink(feedback_sink *sink);
    void emit(const feedback_event &event) const;

    [[nodiscard]] std::size_t sink_count() const;

private:
    blusys::static_vector<feedback_sink *, kMaxSinks> sinks_{};
};

/// Emit feedback on `bus` if non-null (escape hatch replacing `controller::emit_feedback`).
inline void feedback_dispatch(feedback_bus *bus, const feedback_event &event)
{
    if (bus != nullptr) {
        bus->emit(event);
    }
}

const char *feedback_channel_name(feedback_channel channel);
const char *feedback_pattern_name(feedback_pattern pattern);

}  // namespace blusys

#include <blusys/blusys.hpp>

#include "override_capability.hpp"

#include <cstdint>
#include <optional>

namespace override_example::system {

namespace {

constexpr const char *kTag = "override_ref";

}  // namespace

struct app_state {
    std::uint32_t ticks = 0;
    std::uint32_t pulses_seen = 0;
    bool pulse_ready = false;
};

enum class action_tag : std::uint8_t {
    tick,
    pulse_ready,
    pulse,
};

struct action {
    action_tag tag = action_tag::tick;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{
    switch (event.tag) {
    case action_tag::tick:
        ++state.ticks;
        if (state.ticks % 10U == 0U) {
            BLUSYS_LOGI(kTag, "tick #%lu pulses=%lu ready=%s",
                        static_cast<unsigned long>(state.ticks),
                        static_cast<unsigned long>(state.pulses_seen),
                        state.pulse_ready ? "yes" : "no");
        }
        break;

    case action_tag::pulse_ready:
        state.pulse_ready = true;
        BLUSYS_LOGI(kTag, "pulse capability bridged into the reducer");
        break;

    case action_tag::pulse:
        ++state.pulses_seen;
        if (const auto *pulse = ctx.status_of<pulse_capability>(); pulse != nullptr) {
            BLUSYS_LOGI(kTag, "pulse #%lu seen=%lu last=%lu ready=%s",
                        static_cast<unsigned long>(pulse->pulse_count),
                        static_cast<unsigned long>(state.pulses_seen),
                        static_cast<unsigned long>(pulse->last_pulse_ms),
                        pulse->capability_ready ? "yes" : "no");
        } else {
            BLUSYS_LOGI(kTag, "pulse seen=%lu",
                        static_cast<unsigned long>(state.pulses_seen));
        }
        break;
    }
}

std::optional<action> on_event(blusys::event event, app_state & /*state*/)
{
    if (event.source != blusys::event_source::integration) {
        return std::nullopt;
    }

    const auto *cap_event = blusys::event_capability(event);
    if (cap_event == nullptr) {
        return std::nullopt;
    }

    switch (cap_event->raw_event_id) {
    case static_cast<std::uint32_t>(pulse_event::capability_ready):
        return action{.tag = action_tag::pulse_ready};
    case static_cast<std::uint32_t>(pulse_event::pulse):
        return action{.tag = action_tag::pulse};
    default:
        return std::nullopt;
    }
}

void on_tick(blusys::app_ctx &ctx, blusys::app_fx & /*fx*/, app_state & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(action{.tag = action_tag::tick});
}

// Product-local capability: keep this one in product code instead of the catalog.
pulse_capability pulse{pulse_config{.interval_ms = 500, .label = "override-demo"}};
blusys::capability_list_storage capabilities{&pulse};

// Hand-written app_spec: tweak the product-local wiring directly in code.
blusys::app_spec<app_state, action> make_spec()
{
    return blusys::app_spec<app_state, action>{
        .initial_state = {},
        .update = update,
        .on_tick = on_tick,
        .on_event = on_event,
        .tick_period_ms = 200,
        .capabilities = &capabilities,
    };
}

}  // namespace override_example::system

extern "C" void app_main(void)
{
    // Product-specific init zone: add NVS, logging, or hardware setup here.
    blusys::run(override_example::system::make_spec());
}

#if !BLUSYS_DEVICE_BUILD
int main(void)
{
    app_main();
    return 0;
}
#endif

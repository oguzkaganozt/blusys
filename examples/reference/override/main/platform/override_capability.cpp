#include "platform/override_capability.hpp"

namespace override_example::system {

namespace {

constexpr const char *kTag = "override_ref";

}  // namespace

pulse_capability::pulse_capability(const pulse_config &cfg)
    : cfg_(cfg)
{
}

const char *pulse_capability::label() const noexcept
{
    return cfg_.label != nullptr ? cfg_.label : "override-demo";
}

blusys_err_t pulse_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    status_ = pulse_status{};
    status_.capability_ready = true;
    status_.running = true;
    next_pulse_ms_ = 0;

    BLUSYS_LOGI(kTag, "%s ready (interval=%lu ms)",
                label(), static_cast<unsigned long>(cfg_.interval_ms));
    post_event(pulse_event::capability_ready);
    return BLUSYS_OK;
}

void pulse_capability::poll(std::uint32_t now_ms)
{
    if (!status_.running || cfg_.interval_ms == 0) {
        return;
    }

    if (next_pulse_ms_ == 0U) {
        next_pulse_ms_ = now_ms + cfg_.interval_ms;
        return;
    }

    if (now_ms < next_pulse_ms_) {
        return;
    }

    next_pulse_ms_ = now_ms + cfg_.interval_ms;
    ++status_.pulse_count;
    status_.last_pulse_ms = now_ms;
    post_event(pulse_event::pulse);
}

void pulse_capability::stop()
{
    status_.running = false;
    status_.capability_ready = false;
    rt_ = nullptr;
    next_pulse_ms_ = 0;
}

void pulse_capability::post_event(pulse_event event)
{
    post_integration_event(static_cast<std::uint32_t>(event));
}

}  // namespace override_example::system

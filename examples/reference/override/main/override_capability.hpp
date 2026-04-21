#pragma once

#include <blusys/blusys.hpp>

#include <cstdint>

namespace override_example::system {

struct pulse_config {
    std::uint32_t interval_ms = 500;
    const char *label = "override-demo";
};

struct pulse_status : blusys::capability_status_base {
    bool running = false;
    std::uint32_t pulse_count = 0;
    std::uint32_t last_pulse_ms = 0;
};

enum class pulse_event : std::uint32_t {
    pulse = 0x0901,
    capability_ready = 0x09FF,
};

class pulse_capability final : public blusys::capability_base {
public:
    static constexpr blusys::capability_kind kind_value = blusys::capability_kind::custom;

    explicit pulse_capability(const pulse_config &cfg);

    [[nodiscard]] blusys::capability_kind kind() const override { return kind_value; }
    [[nodiscard]] const pulse_status &status() const { return status_; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

private:
    [[nodiscard]] const char *label() const noexcept;
    void post_event(pulse_event event);

    pulse_config cfg_;
    pulse_status status_{};
    std::uint32_t next_pulse_ms_ = 0;
};

}  // namespace override_example::system

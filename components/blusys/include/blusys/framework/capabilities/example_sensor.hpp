#pragma once

// Canonical capability template — minimal, reads one value.
// This is the shape the scaffolder emits and the reference every new
// capability header is measured against (≤60 LOC of class shell).

#include "blusys/framework/capabilities/capability.hpp"

#include <cstdint>

namespace blusys {

struct example_sensor_config {
    std::uint32_t sample_interval_ms = 1000;
};

struct example_sensor_status : capability_status_base {
    std::int32_t last_value = 0;
    std::uint32_t sample_count = 0;
};

class example_sensor_capability final : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::example_sensor;

    explicit example_sensor_capability(const example_sensor_config &cfg);
    ~example_sensor_capability() override;

    [[nodiscard]] capability_kind kind() const override { return kind_value; }

    blusys_err_t start(blusys::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    [[nodiscard]] const example_sensor_status &status() const { return status_; }

    // Typed effect — read the latest sample.
    blusys_err_t read(std::int32_t *out);

private:
    example_sensor_config cfg_;
    example_sensor_status status_{};
    std::uint32_t         last_sample_ms_ = 0;
};

}  // namespace blusys

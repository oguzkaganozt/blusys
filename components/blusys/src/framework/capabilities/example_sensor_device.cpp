#include "blusys/framework/capabilities/example_sensor.hpp"

namespace blusys {

// Minimal device implementation — sample count grows once per sample interval.
// A real sensor would back this with an ADC read through `blusys/hal/adc.h`.
example_sensor_capability::example_sensor_capability(const example_sensor_config &cfg)
    : cfg_(cfg) {}

example_sensor_capability::~example_sensor_capability() = default;

blusys_err_t example_sensor_capability::start(runtime &rt)
{
    rt_ = &rt;
    status_.capability_ready = true;
    status_.last_value       = 0;
    status_.sample_count     = 0;
    last_sample_ms_          = 0;
    return BLUSYS_OK;
}

void example_sensor_capability::poll(std::uint32_t now_ms)
{
    if ((now_ms - last_sample_ms_) < cfg_.sample_interval_ms) {
        return;
    }
    last_sample_ms_        = now_ms;
    status_.last_value     = static_cast<std::int32_t>(status_.sample_count);
    status_.sample_count  += 1;
}

void example_sensor_capability::stop()
{
    status_.capability_ready = false;
    rt_ = nullptr;
}

blusys_err_t example_sensor_capability::read(std::int32_t *out)
{
    if (out == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    *out = status_.last_value;
    return BLUSYS_OK;
}

}  // namespace blusys

// app_headless_demo — Phase 1 blusys::app headless sensor node.
//
// A temperature monitoring product that demonstrates the headless path:
//   - same app_spec / update() model as the interactive demo
//   - on_tick simulates sensor readings
//   - alarm threshold triggers action re-dispatch
//   - no UI, no LVGL — pure reducer logic
//
// This file contains ZERO framework plumbing. The BLUSYS_APP_MAIN_HEADLESS
// macro handles everything.

#include "blusys/app/app.hpp"
#include "blusys/log.h"

#include <cstdint>

namespace {

constexpr const char *kTag = "sensor_node";

// ---- state ----

struct SensorState {
    float    temperature   = 20.0f;
    float    humidity      = 45.0f;
    uint32_t reading_count = 0;
    bool     alarm_active  = false;
};

// ---- actions ----

enum class ActionTag : std::uint8_t {
    sensor_reading,
    check_alarm,
    reset_alarm,
};

struct SensorReading {
    float temperature;
    float humidity;
};

struct Action {
    ActionTag     tag;
    SensorReading reading;
};

// ---- reducer ----

void update(blusys::app::app_ctx &ctx, SensorState &state, const Action &action)
{
    switch (action.tag) {
    case ActionTag::sensor_reading:
        state.temperature = action.reading.temperature;
        state.humidity    = action.reading.humidity;
        state.reading_count++;

        BLUSYS_LOGI(kTag, "reading #%lu: temp=%.1f humidity=%.1f",
                    static_cast<unsigned long>(state.reading_count),
                    static_cast<double>(state.temperature),
                    static_cast<double>(state.humidity));

        if (state.temperature > 40.0f && !state.alarm_active) {
            ctx.dispatch(Action{.tag = ActionTag::check_alarm, .reading = {}});
        }
        if (state.temperature <= 35.0f && state.alarm_active) {
            ctx.dispatch(Action{.tag = ActionTag::reset_alarm, .reading = {}});
        }
        break;

    case ActionTag::check_alarm:
        state.alarm_active = true;
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::warning);
        BLUSYS_LOGW(kTag, "ALARM: temperature %.1f exceeds threshold",
                    static_cast<double>(state.temperature));
        break;

    case ActionTag::reset_alarm:
        state.alarm_active = false;
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::success);
        BLUSYS_LOGI(kTag, "alarm cleared — temperature %.1f normal",
                    static_cast<double>(state.temperature));
        break;
    }
}

// ---- on_tick: simulate sensor readings ----

void on_tick(blusys::app::app_ctx &ctx, SensorState &state, std::uint32_t now_ms)
{
    (void)state;

    // Simulate a temperature curve: rises for 10 readings, then drops.
    // The cycle repeats every 20 ticks.
    const uint32_t cycle = (now_ms / 1000) % 20;
    float temp;
    if (cycle < 10) {
        temp = 20.0f + static_cast<float>(cycle) * 3.0f;  // 20 → 47
    } else {
        temp = 47.0f - static_cast<float>(cycle - 10) * 3.0f;  // 47 → 20
    }
    const float hum = 45.0f + static_cast<float>(cycle % 5) * 2.0f;

    ctx.dispatch(Action{
        .tag     = ActionTag::sensor_reading,
        .reading = {.temperature = temp, .humidity = hum},
    });
}

}  // namespace

// ---- app definition ----

static const blusys::app::app_spec<SensorState, Action> spec{
    .initial_state  = {},
    .update         = update,
    .on_init        = nullptr,
    .on_tick        = on_tick,
    .map_intent     = nullptr,
    .tick_period_ms = 1000,
};

BLUSYS_APP_MAIN_HEADLESS(spec)

#pragma once

#include <cstdint>

namespace blusys {

// Timing and intensity parameters for a single feedback pattern.
// Interpretation is driver-specific: haptic sinks use duration_ms and
// intensity to drive a vibration motor; audio sinks map them to tone
// length and volume; visual sinks map them to flash duration and
// brightness.
struct feedback_timing {
    std::uint16_t duration_ms;   // 0 = sink default
    std::uint8_t  intensity;     // 0–255, driver-interpreted
    std::uint8_t  repeat;        // 0 = once
};

// A complete mapping from each feedback_pattern to its timing.
// Products can define their own, or use the framework-owned presets.
struct feedback_preset {
    feedback_timing click;
    feedback_timing focus;
    feedback_timing confirm;
    feedback_timing success;
    feedback_timing warning;
    feedback_timing error;
    feedback_timing notification;
};

// Expressive preset — stronger, longer patterns for consumer/controller
// products that want tactile, characterful feedback.
const feedback_preset &feedback_preset_expressive();

// Operational preset — minimal, short, professional patterns for
// industrial/panel products that want unobtrusive feedback.
const feedback_preset &feedback_preset_operational();

}  // namespace blusys

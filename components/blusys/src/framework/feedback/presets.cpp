#include "blusys/framework/feedback/presets.hpp"

namespace blusys {

const feedback_preset &feedback_preset_expressive()
{
    static const feedback_preset preset{
        .click        = {.duration_ms = 15,  .intensity = 180, .repeat = 0},
        .focus        = {.duration_ms = 8,   .intensity = 80,  .repeat = 0},
        .confirm      = {.duration_ms = 30,  .intensity = 220, .repeat = 0},
        .success      = {.duration_ms = 50,  .intensity = 200, .repeat = 0},
        .warning      = {.duration_ms = 60,  .intensity = 160, .repeat = 2},
        .error        = {.duration_ms = 80,  .intensity = 255, .repeat = 3},
        .notification = {.duration_ms = 40,  .intensity = 140, .repeat = 1},
    };
    return preset;
}

const feedback_preset &feedback_preset_operational()
{
    static const feedback_preset preset{
        .click        = {.duration_ms = 8,   .intensity = 100, .repeat = 0},
        .focus        = {.duration_ms = 5,   .intensity = 40,  .repeat = 0},
        .confirm      = {.duration_ms = 15,  .intensity = 120, .repeat = 0},
        .success      = {.duration_ms = 20,  .intensity = 100, .repeat = 0},
        .warning      = {.duration_ms = 30,  .intensity = 120, .repeat = 1},
        .error        = {.duration_ms = 40,  .intensity = 180, .repeat = 2},
        .notification = {.duration_ms = 20,  .intensity = 80,  .repeat = 0},
    };
    return preset;
}

}  // namespace blusys

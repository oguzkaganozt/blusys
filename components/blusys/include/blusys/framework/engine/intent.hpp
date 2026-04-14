#pragma once

#include <cstdint>

namespace blusys {

enum class intent : std::uint8_t {
    press,
    long_press,
    release,
    confirm,
    cancel,
    increment,
    decrement,
    focus_next,
    focus_prev,
};

enum class app_event_kind : std::uint8_t {
    intent,
    integration,
};

struct app_event {
    app_event_kind kind = app_event_kind::intent;
    std::uint32_t  id = 0;
    std::uint32_t  code = 0;
    const void    *payload = nullptr;
};

[[nodiscard]] constexpr app_event make_intent_event(intent value,
                                                    std::uint32_t source_id = 0,
                                                    const void *payload = nullptr)
{
    return {
        .kind = app_event_kind::intent,
        .id = source_id,
        .code = static_cast<std::uint32_t>(value),
        .payload = payload,
    };
}

[[nodiscard]] constexpr app_event make_integration_event(std::uint32_t event_id,
                                                         std::uint32_t event_code = 0,
                                                         const void *payload = nullptr)
{
    return {
        .kind = app_event_kind::integration,
        .id = event_id,
        .code = event_code,
        .payload = payload,
    };
}

[[nodiscard]] constexpr intent app_event_intent(const app_event &event)
{
    return static_cast<intent>(event.code);
}

}  // namespace blusys

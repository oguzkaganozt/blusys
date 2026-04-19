#pragma once

#include <cstdint>

namespace blusys {

// User-input intent values. Emitted by input capabilities (buttons, encoder,
// touch) with `source == event_source::intent` and the intent value carried in
// the event's `kind` field.
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

// Which subsystem produced an event. Scopes the meaning of `kind` and `id`
// on `app_event`.
enum class event_source : std::uint8_t {
    intent,       // kind is a blusys::intent value; id is a button/encoder source_id
    integration,  // kind is the raw integration event_code; id is the event category
};

// The single on-bus event type. Framework intents and capability events travel
// here; `source` disambiguates the meaning of kind/id.
struct app_event {
    event_source  source  = event_source::intent;
    std::uint32_t kind    = 0;
    std::uint32_t id      = 0;
    const void   *payload = nullptr;
};

using event = app_event;

[[nodiscard]] constexpr app_event make_intent_event(intent value,
                                                    std::uint32_t source_id = 0,
                                                    const void *payload = nullptr)
{
    return {
        .source  = event_source::intent,
        .kind    = static_cast<std::uint32_t>(value),
        .id      = source_id,
        .payload = payload,
    };
}

[[nodiscard]] constexpr app_event make_integration_event(std::uint32_t event_id,
                                                         std::uint32_t event_code = 0,
                                                         const void *payload = nullptr)
{
    return {
        .source  = event_source::integration,
        .kind    = event_code,
        .id      = event_id,
        .payload = payload,
    };
}

[[nodiscard]] constexpr intent app_event_intent(const app_event &event)
{
    return static_cast<intent>(event.kind);
}

[[nodiscard]] constexpr intent event_intent(const event &event)
{
    return app_event_intent(event);
}

}  // namespace blusys

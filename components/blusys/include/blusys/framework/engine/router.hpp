#pragma once

#include <cstdint>

namespace blusys {

enum class route_command_type : std::uint8_t {
    set_root,
    push,
    replace,
    pop,
    show_overlay,
    hide_overlay,
};

struct route_command {
    route_command_type type = route_command_type::set_root;
    std::uint32_t      id = 0;
    const void        *params = nullptr;
    std::uint32_t      transition = 0;
};

class route_sink {
public:
    virtual ~route_sink() = default;

    virtual void submit(const route_command &command) = 0;
};

/// Submit a route to `sink` if non-null (escape hatch replacing `controller::submit_route`).
inline void route_dispatch(route_sink *sink, const route_command &command)
{
    if (sink != nullptr) {
        sink->submit(command);
    }
}

const char *route_command_type_name(route_command_type type);

namespace route {

[[nodiscard]] constexpr route_command set_root(std::uint32_t route_id,
                                               const void *params = nullptr,
                                               std::uint32_t transition = 0)
{
    return {
        .type = route_command_type::set_root,
        .id = route_id,
        .params = params,
        .transition = transition,
    };
}

[[nodiscard]] constexpr route_command push(std::uint32_t route_id,
                                           const void *params = nullptr,
                                           std::uint32_t transition = 0)
{
    return {
        .type = route_command_type::push,
        .id = route_id,
        .params = params,
        .transition = transition,
    };
}

[[nodiscard]] constexpr route_command replace(std::uint32_t route_id,
                                              const void *params = nullptr,
                                              std::uint32_t transition = 0)
{
    return {
        .type = route_command_type::replace,
        .id = route_id,
        .params = params,
        .transition = transition,
    };
}

[[nodiscard]] constexpr route_command pop(std::uint32_t transition = 0)
{
    return {
        .type = route_command_type::pop,
        .id = 0,
        .params = nullptr,
        .transition = transition,
    };
}

[[nodiscard]] constexpr route_command show_overlay(std::uint32_t overlay_id,
                                                   const void *params = nullptr,
                                                   std::uint32_t transition = 0)
{
    return {
        .type = route_command_type::show_overlay,
        .id = overlay_id,
        .params = params,
        .transition = transition,
    };
}

[[nodiscard]] constexpr route_command hide_overlay(std::uint32_t overlay_id)
{
    return {
        .type = route_command_type::hide_overlay,
        .id = overlay_id,
        .params = nullptr,
        .transition = 0,
    };
}

}  // namespace route

}  // namespace blusys

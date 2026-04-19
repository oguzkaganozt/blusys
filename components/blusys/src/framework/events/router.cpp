#include "blusys/framework/events/router.hpp"

namespace blusys {

const char *route_command_type_name(route_command_type type)
{
    switch (type) {
    case route_command_type::set_root:
        return "set_root";
    case route_command_type::push:
        return "push";
    case route_command_type::replace:
        return "replace";
    case route_command_type::pop:
        return "pop";
    case route_command_type::show_overlay:
        return "show_overlay";
    case route_command_type::hide_overlay:
        return "hide_overlay";
    }

    return "unknown";
}

}  // namespace blusys

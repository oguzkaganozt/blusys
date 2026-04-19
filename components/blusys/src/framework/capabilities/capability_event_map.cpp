#include "blusys/framework/capabilities/event.hpp"
#include "blusys/framework/capabilities/event_table.hpp"

#include <algorithm>

namespace blusys {

bool map_integration_event(std::uint32_t event_id, std::uint32_t event_code,
                           capability_event *out) noexcept
{
    if (out == nullptr) {
        return false;
    }

    const auto *begin = kEventTable;
    const auto *end   = kEventTable + kEventTableSize;
    const auto *it = std::lower_bound(
        begin, end, event_id,
        [](const event_mapping &m, std::uint32_t id) { return m.id < id; });

    if (it == end || it->id != event_id) {
        return false;
    }

    *out = capability_event{};
    out->tag = it->tag;
    out->raw_event_id = event_id;
    out->raw_event_code = event_code;
    if (it->carry_code_as_value) {
        out->value = static_cast<std::int32_t>(event_code);
    }
    return true;
}

}  // namespace blusys

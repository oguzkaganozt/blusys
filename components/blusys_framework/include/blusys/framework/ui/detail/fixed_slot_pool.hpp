#pragma once

// Shared fixed-capacity callback slot pool logic for stock widgets — see
// widget-author-guide.md (Camp 2, pool sizing).
//
// Slot types must be POD structs with an `in_use` (bool) member. Release uses
// zero-initialization to clear all fields. Do not use these helpers for ad-hoc layouts.

#include "blusys/log.h"

#include <cassert>
#include <cstddef>

namespace blusys::ui::detail {

// Returns the first free slot, or nullptr only when NDEBUG is defined (assert
// stripped): debug builds abort after logging pool exhaustion.
template <typename Slot, std::size_t N>
Slot *acquire_ui_slot(Slot (&slots)[N], const char *tag, const char *raise_macro_name)
{
    for (auto &s : slots) {
        if (!s.in_use) {
            s.in_use = true;
            return &s;
        }
    }
    BLUSYS_LOGE(tag,
                "slot pool exhausted (size=%d) — raise %s",
                static_cast<int>(N),
                raise_macro_name);
    assert(false);
    return nullptr;
}

// Clears Camp-2 slot fields via zero-init (released state for POD slots).
template <typename Slot>
void release_ui_slot(Slot *slot)
{
    if (slot != nullptr) {
        *slot = {};
    }
}

}  // namespace blusys::ui::detail

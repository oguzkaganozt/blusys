#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/containers.hpp"

#include <cstddef>

#include "lvgl.h"

namespace blusys {

// Manages a stack of focus scopes.
//
// When a screen is loaded, its widgets are added to a focus scope.
// When a modal or overlay opens, a new scope is pushed that captures
// focus to the overlay's widget tree. When it closes, focus returns
// to the previous scope.
//
// This replaces the ad-hoc group management that was previously
// scattered across screen_registry and input_bridge.
class focus_scope_manager {
public:
    static constexpr std::size_t kMaxScopes = 4;

    // Push a new focus scope for the given container.
    // Creates a new lv_group_t, populates it from the container's
    // focusable children, and binds it to all encoder indevs.
    // The previous scope is suspended (its group is unbound from indevs).
    // Returns true on success.
    bool push_scope(lv_obj_t *container);

    // Pop the top scope. Destroys its group and restores the previous
    // scope's group to all encoder indevs.
    void pop_scope();

    // Re-walk the current scope's container and rebuild its focus group.
    // Call after adding or removing widgets dynamically.
    void refresh_current();

    // Get the current active group, or nullptr if no scope is active.
    [[nodiscard]] lv_group_t *current_group() const;

    // Pop all scopes (e.g., on full screen change).
    void reset();

    // Number of active scopes.
    [[nodiscard]] std::size_t depth() const { return stack_.size(); }

private:
    struct scope_entry {
        lv_obj_t   *container = nullptr;
        lv_group_t *group     = nullptr;
    };

    void bind_group_to_encoders(lv_group_t *group);

    blusys::static_vector<scope_entry, kMaxScopes> stack_{};
};

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI

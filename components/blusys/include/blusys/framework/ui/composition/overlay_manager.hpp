#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/containers.hpp"
#include "blusys/framework/events/router.hpp"
#include "blusys/framework/ui/input/focus_scope.hpp"
#include "blusys/framework/ui/widgets/overlay.hpp"

#include <cstdint>

namespace blusys {

// Manages overlay widgets and applies route commands (show_overlay / hide_overlay)
// to real LVGL overlay handles. Replaces the earlier logging-only route sink.
//
// Products register overlays by ID during on_init. When the reducer calls
// ctx.fx().nav.show_overlay(id) or ctx.fx().nav.hide_overlay(id), the route
// command flows through the framework runtime into this sink, which calls the
// actual blusys::overlay_show/hide.

class overlay_manager final : public blusys::route_sink {
public:
    static constexpr std::size_t kMaxOverlays = 8;

    bool register_overlay(std::uint32_t id, lv_obj_t *overlay);

    // Bind a focus scope manager for focus trapping on overlay show/hide.
    void bind_focus_scope(blusys::focus_scope_manager *fsm) { focus_scope_ = fsm; }

    void submit(const blusys::route_command &command) override;

private:
    struct entry {
        std::uint32_t id     = 0;
        lv_obj_t     *widget = nullptr;
    };

    lv_obj_t *find(std::uint32_t id) const;

    blusys::static_vector<entry, kMaxOverlays> overlays_{};
    blusys::focus_scope_manager           *focus_scope_ = nullptr;
};

// Convenience: create an overlay widget and register it with the manager.
lv_obj_t *overlay_create(lv_obj_t *parent,
                         std::uint32_t route_id,
                         const blusys::overlay_config &config,
                         overlay_manager &mgr);

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
